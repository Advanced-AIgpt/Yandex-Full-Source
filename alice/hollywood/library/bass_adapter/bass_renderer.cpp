#include "bass_renderer.h"

#include <alice/hollywood/library/player_features/player_features.h>
#include <alice/hollywood/library/response/push.h>

#include <alice/library/analytics/common/utils.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/billing/defs.h>
#include <alice/library/json/json.h>
#include <alice/library/music/defs.h>

#include <alice/megamind/protos/analytics/scenarios/music/music.pb.h>
#include <alice/megamind/protos/analytics/scenarios/vins/vins.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <util/string/join.h>

namespace NAlice::NHollywood {

namespace {

const THashSet<TStringBuf> PLAYER_FORMS = {
    TStringBuf("personal_assistant.scenarios.player_next_track"),
    TStringBuf("personal_assistant.scenarios.player_previous_track"),
    TStringBuf("personal_assistant.scenarios.player_shuffle"),
    TStringBuf("personal_assistant.scenarios.player_continue"),
};

constexpr TStringBuf NLG_PLAYER_NAME = "render_player";

const TString TEACH_ME{TStringBuf("personal_assistant.scenarios.teach_me")};
const TString PLACEHOLDERS{TStringBuf("placeholders")};

TVector<const NJson::TJsonValue*> FilterBlocksByType(const NJson::TJsonValue::TArray& bassResponseBlocks,
                                                     const TStringBuf blockType) {
    TVector<const NJson::TJsonValue*> result(Reserve(bassResponseBlocks.size()));
    for (const auto& block : bassResponseBlocks) {
        const NJson::TJsonValue* typePtr = nullptr;
        Y_ENSURE(block.GetValuePointer("type", &typePtr), "block must have `type` field");
        if (typePtr->GetStringSafe() == blockType) {
            result.push_back(&block);
        }
    }
    return result;
}

void EnrichNlgData(const TScenarioBaseRequestWrapper& request,
                    const TScenarioInputWrapper& input, TNlgData& nlgData) {
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    nlgData.ReqInfo["experiments"].SetType(NJson::JSON_MAP);
    for (const auto& [name, value] : request.ExpFlags()) {
        if (value.Defined()) {
            nlgData.ReqInfo["experiments"][name] = *value;
        }
    }
    nlgData.ReqInfo["utterance"]["text"] = input.Utterance();
    nlgData.Context["has_alicesdk_player"] = request.Interfaces().GetHasMusicSdkClient();
}

void AddAttentionsToNlgData(const NJson::TJsonValue::TArray& bassResponseBlocks, TNlgData& nlgData) {
    for (const auto* blockPtr : FilterBlocksByType(bassResponseBlocks, "attention")) {
        const auto& block = *blockPtr;
        Y_ENSURE(block.Has("attention_type"), "attention block must have `attention_type` field");
        const auto& attentionType = block["attention_type"].GetString();
        if (block.Has("data")) {
            nlgData.Context["attentions"][attentionType]["data"] = block["data"];
        } else {
            nlgData.Context["attentions"][attentionType] = true;
        }
    }
}

NScenarios::TDirective CreateTypeTextDirective(const TString& name, const TString& text) {
    NScenarios::TDirective directive;
    NScenarios::TTypeTextDirective* typeTextDirective = directive.MutableTypeTextDirective();
    typeTextDirective->SetText(text);
    typeTextDirective->SetName(name);
    return directive;
}

NScenarios::TDirective CreateTypeTextSilentDirective(const TString& name, const TString& text) {
    NScenarios::TDirective directive;
    NScenarios::TTypeTextSilentDirective* typeTextSilentDirective = directive.MutableTypeTextSilentDirective();
    typeTextSilentDirective->SetText(text);
    typeTextSilentDirective->SetName(name);
    return directive;
}

NScenarios::TDirective CreateOpenUriDirective(const TString& name, const TString& uri, const TMaybe<TStringBuf> screenId) {
    NScenarios::TDirective directive;
    NScenarios::TOpenUriDirective* openUriDirective = directive.MutableOpenUriDirective();
    openUriDirective->SetUri(uri);
    openUriDirective->SetName(name);
    if (screenId.Defined()) {
        openUriDirective->SetScreenId(screenId->data(), screenId->size());
    }
    return directive;
}

bool IsPlayerFormFromMusicCommands(const NJson::TJsonValue& bassResponse)
{
    const auto& form = bassResponse["form"]["name"].GetString();
    if (PLAYER_FORMS.contains(form) && bassResponse["form"]["slots"].IsArray()) {
        for (const auto& slot : bassResponse["form"]["slots"].GetArray()) {
            if (slot["name"] == "music_player_only") {
                return true;
            }
        }
    }
    return false;
}

TBassResponseRenderer::TServerDirectiveParsersMapping InitPreErrorServerDirectiveParsers() {
    TBassResponseRenderer::TServerDirectiveParsersMapping parsers;
    parsers["send_push"] = MakeHolder<NImpl::TSendPushDirectiveParser>();
    return parsers;
}

TBassResponseRenderer::TServerDirectiveParsersMapping InitPostErrorServerDirectiveParsers() {
    TBassResponseRenderer::TServerDirectiveParsersMapping parsers;
    parsers["update_datasync"] = MakeHolder<NImpl::TUpdateDatasyncDirectiveParser>();
    return parsers;
}

class TLazyAnalyticsInfoBuilder {
public:
    TLazyAnalyticsInfoBuilder(const NJson::TJsonValue& bassResponse, TResponseBodyBuilder& bodyBuilder)
        : BodyBuilder_(bodyBuilder)
    {
        if (auto analyticsInfo = NScenarios::GetAnalyticsInfoFromBassResponse(bassResponse)) {
            if (BodyBuilder_.HasAnalyticsInfoBuilder()) {
                BodyBuilder_.GetResponseBody().MutableAnalyticsInfo()->MergeFrom(std::move(*analyticsInfo));
            } else {
                BodyBuilder_.CreateAnalyticsInfoBuilder(std::move(*analyticsInfo));
            }
        }
    }

    NScenarios::IAnalyticsInfoBuilder& Get() {
        return BodyBuilder_.GetOrCreateAnalyticsInfoBuilder();
    }
    NScenarios::IAnalyticsInfoBuilder* operator->() {
        return &Get();
    }
    void FillAnalyticsInfo(const TString& analyticsIntentName, const TString& analyticsScenarioName) {
        if (analyticsIntentName) {
            Get().SetIntentName(analyticsIntentName);
        }
        if (analyticsScenarioName) {
            Get().SetProductScenarioName(analyticsScenarioName);
        }
    }

private:
    TResponseBodyBuilder& BodyBuilder_;
};

} // namespace

namespace NImpl {

bool TUpdateDatasyncDirectiveParser::MakeDirective(const NJson::TJsonValue& data, TResponseBodyBuilder& bodyBuilder) const {
    NScenarios::TServerDirective directive;
    auto& mutableUpdateDS = *directive.MutableUpdateDatasyncDirective();
    mutableUpdateDS.SetKey(data["key"].GetStringSafe());
    mutableUpdateDS.SetStringValue(data["value"].GetStringSafe());
    NScenarios::TUpdateDatasyncDirective_EDataSyncMethod method;
    if (!TUpdateDatasyncDirective_EDataSyncMethod_Parse(to_title(data["method"].GetStringSafe()), &method)) {
        return false;
    }
    mutableUpdateDS.SetMethod(method);
    bodyBuilder.AddServerDirective(std::move(directive));
    return true;
}

bool TSendPushDirectiveParser::MakeDirective(const NJson::TJsonValue& data, TResponseBodyBuilder& bodyBuilder) const {
    TPushDirectiveBuilder{data["title"].GetStringSafe(),
                          data["text"].GetStringSafe(),
                          data["url"].GetStringSafe(),
                          data["tag"].GetStringSafe()}
        .SetPushId(data["id"].GetStringSafe())
        .SetThrottlePolicy(data["throttle"].GetStringSafe())
        .BuildTo(bodyBuilder);
    return true;
}

TMaybe<TFrame> ParseBassForm(const NJson::TJsonValue& bassForm) {
    const auto name = bassForm["name"].GetString();
    if (!name) {
        return Nothing();
    }

    auto frame = MakeMaybe<TFrame>(name);
    for (const auto& slot : bassForm["slots"].GetArray()) {
        const auto name = slot["name"].GetString();
        // NOTE(a-square): we intentionally only get string values to avoid passing
        // the answer slot to the form to Begemot (it wouldn't know what to do with it)
        frame->AddSlot(TSlot{name, slot["type"].GetString(),
                             TSlot::TValue{slot["value"].GetString()}});
    }
    return frame;
}

bool AddServerDirective(TResponseBodyBuilder& bodyBuilder,
                        TRTLogger& logger,
                        const IServerDirectiveParser& directiveParser,
                        const NJson::TJsonValue& data)
{
    if (!directiveParser.MakeDirective(data, bodyBuilder)) {
        LOG_WARNING(logger) << "failed parse " << directiveParser.GetDirectiveName() << ": " << data;
        return false;
    }
    return true;
}

} // namespace NImpl

const TBassResponseRenderer::TServerDirectiveParsersMapping TBassResponseRenderer::PreErrorServerDirectiveParsers = InitPreErrorServerDirectiveParsers();
const TBassResponseRenderer::TServerDirectiveParsersMapping TBassResponseRenderer::PostErrorServerDirectiveParsers = InitPostErrorServerDirectiveParsers();

TBassResponseRenderer::TBassResponseRenderer(const TScenarioBaseRequestWrapper& baseRequest,
                                             const TScenarioInputWrapper& input,
                                             IResponseBuilder& builder,
                                             TRTLogger& logger,
                                             bool suggestAutoAction,
                                             bool reduceWhitespaceInCards,
                                             const bool addFistTrackObject)
    : BaseRequest_{baseRequest}
    , Input_{input}
    , ReduceWhitespaceInCards_(reduceWhitespaceInCards)
    , Builder_{builder}
    , Logger_{logger}
    , NlgData_{CreateNlgData()}
    , SuggestAutoAction_(suggestAutoAction)
    , AddFirstTrackObject_(addFistTrackObject)
{
}

void TBassResponseRenderer::Render(const TStringBuf nlgTemplateName,
                                   const TStringBuf nlgPhraseName,
                                   const NJson::TJsonValue& bassResponse,
                                   const TString& analyticsIntentName,
                                   const TString& analyticsScenarioName,
                                   bool processSuggestsOnError) {
    return Render(nlgTemplateName, bassResponse, analyticsIntentName, analyticsScenarioName, nlgPhraseName,
                  processSuggestsOnError);
}

void TBassResponseRenderer::Render(const TStringBuf nlgTemplateName,
                                   const NJson::TJsonValue& bassResponse,
                                   const TString& analyticsIntentName,
                                   const TString& analyticsScenarioName,
                                   const TStringBuf nlgPhraseName,
                                   bool processSuggestsOnError) {
    Y_ENSURE(bassResponse.Has("blocks"), "Bass response has no `blocks` key");

    TNlgData nlgData{NlgData_};

    // NOTE(a-square): we must use the raw BASS form to avoid censoring the answer slot
    if (bassResponse.Has("form")) {
        nlgData.Form = bassResponse["form"];
    }
    if (IsPlayerFormFromMusicCommands(bassResponse)) {
        auto playerFeatures = NAlice::NHollywood::CalcPlayerFeaturesFor(
            Logger_, BaseRequest_, {EPlayer::MUSIC_PLAYER, EPlayer::BLUETOOTH_PLAYER});
        Builder_.AddPlayerFeatures(std::move(playerFeatures));
    } else {
        LOG_INFO(Logger_) << "Skipping add PlayerFeatures...";
    }

    const auto& blocks = bassResponse["blocks"].GetArray();
    AddAttentionsToNlgData(blocks, nlgData);

    LOG_INFO(Logger_) << "Parsing the BASS form";
    const auto form = NImpl::ParseBassForm(bassResponse["form"]);

    LOG_INFO(Logger_) << "Creating the body builder";
    auto& bodyBuilder = Builder_.GetOrCreateResponseBodyBuilder(form.Get());
    TLazyAnalyticsInfoBuilder analyticsInfoBuilder{bassResponse, bodyBuilder};

    LOG_INFO(Logger_) << "Rendering pre-error server directives";
    for (const auto* blockPtr : FilterBlocksByType(blocks, "uniproxy-action")) {
        const auto& block = *blockPtr;
        Y_ENSURE(block.Has("command_type"));
        const auto& commandType = block["command_type"].GetString();
        const auto& data = block["data"];
        Y_ENSURE(data.IsDefined());
        if (PreErrorServerDirectiveParsers.contains(commandType)) {
            NImpl::AddServerDirective(bodyBuilder, Logger_, *PreErrorServerDirectiveParsers.at(commandType), data);
        }
    }

    // one user-friendly error is enough
    const auto errorBlocks = FilterBlocksByType(blocks, "error");
    if (errorBlocks) {
        const auto& block = *errorBlocks[0];
        const auto& errorType = block["error"]["type"].GetStringSafe();
        const auto& errorCode = block["data"]["code"].GetString();

        LOG_INFO(Logger_) << "Rendering the error block";
        nlgData.Context["error"]["data"]["code"] = errorCode;
        auto phraseName = Join("__", "render_error", errorType);

        if (errorCode == NAlice::NMusic::ERROR_CODE_EXTRA_PROMO_PERIOD_AVAILABLE) {
            if (const auto expiresSlot = form->FindSlot(NAlice::NBilling::EXTRA_PROMO_PERIOD_EXPIRES_DATE)) {
                nlgData.Context[NAlice::NBilling::EXTRA_PROMO_PERIOD_EXPIRES_DATE] = expiresSlot->Value.AsString();
            }
        }

        bodyBuilder.AddRenderedTextWithButtonsAndVoice(nlgTemplateName, phraseName, /* buttons= */ {}, nlgData);

        analyticsInfoBuilder.FillAnalyticsInfo(analyticsIntentName, analyticsScenarioName);

        NScenarios::TAnalyticsInfo::TObject errorMeta;
        errorMeta.MutableVinsErrorMeta()->SetType(errorType);
        analyticsInfoBuilder->AddObject(errorMeta);

        // ALICE-8059: News scenario add suggests to error response.
        if (!processSuggestsOnError) {
            return;
        }
    }

    // TODO(a-square): modularize this
    TString firstTrackId;
    TString firstTrackGenre;
    TString firstTrackDuration;
    TString firstTrackAlbumType;
    TString firstTrackHumanReadable;

    bool unsupportedDirective = false;

    LOG_INFO(Logger_) << "Rendering commands";
    for (const auto* blockPtr : FilterBlocksByType(blocks, "command")) {
        const auto& block = *blockPtr;
        Y_ENSURE(block.Has("command_type"));
        const auto& commandType = block["command_type"].GetString();
        const auto& commandSubType = block["command_sub_type"].GetStringSafe(commandType);

        const auto& data = block["data"];
        Y_ENSURE(data.IsDefined());

        // TODO(a-square): this is a partial port of VINS's to_autoapp_directives function,
        // remove this after the old Yandex Auto client is discontinued.
        // TODO(a-square): modularize this!
        if (BaseRequest_.ClientInfo().IsOldYaAuto()) {
            if (commandType == TStringBuf("open_uri") &&
                data[TStringBuf("uri")].GetString().StartsWith(TStringBuf("https://radio.yandex.ru/user/onyourwave"))
            ) {
                NJson::TJsonValue value;
                value["params"][TStringBuf("app")] = "yandexradio";
                value["application"] = "car";
                value["intent"] = "launch";
                bodyBuilder.AddClientActionDirective("car", "car_launch", value);
            } else {
                LOG_WARNING(Logger_) << "Old auto can't handle directive: " << data[TStringBuf("uri")].GetString();
                unsupportedDirective = true;
            }
        } else {
            bodyBuilder.AddClientActionDirective(commandType, commandSubType, data);
        }

        firstTrackId = data["first_track_id"].GetString();
        firstTrackGenre = data["first_track_genre"].GetString();
        firstTrackDuration = data["first_track_duration"].GetString();
        firstTrackAlbumType = data["first_track_album_type"].GetString();
        firstTrackHumanReadable = data["first_track_human_readable"].GetString();
    }

    if (unsupportedDirective) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(nlgTemplateName, "render_inability", /* buttons = */ {}, nlgData);

        LOG_INFO(Logger_) << "Filling analytics info";
        analyticsInfoBuilder.FillAnalyticsInfo(TEACH_ME, PLACEHOLDERS);
        return;
    }

    if (!form.Defined()) {
        LOG_WARNING(Logger_) << "No error blocks and no form in the bass response";
    }

    LOG_INFO(Logger_) << "Rendering suggest blocks";
    for (const auto* blockPtr : FilterBlocksByType(blocks, "suggest")) {
        const auto& block = *blockPtr;

        const auto& suggestType = block["suggest_type"].GetString();
        if (suggestType == TStringBuf("search_internet_fallback")) {
            const auto& slots = block[TStringBuf("form_update")][TStringBuf("slots")].GetArray();
            if (const auto* querySlotPtr = FindIfPtr(slots, [](const auto& slot) {
                return slot[TStringBuf("name")] == TStringBuf("query");
            })) {
                // process search suggest, if any
                const auto& query = (*querySlotPtr)[TStringBuf("value")].GetString();
                bodyBuilder.AddSearchSuggest().Title(query).Query(query);
            } else {
                LOG_ERROR(Logger_) << "Got invalid search suggest: " << block;
            }
            continue;
        }

        if (suggestType.StartsWith(TStringBuf("music__suggest_"))) {
            auto suggest = CreateSuggest(nlgTemplateName, block, nlgData);
            if (suggest.Defined()) {
                bodyBuilder.AddRenderedSuggest(std::move(suggest.GetRef()));
            }
            continue;
        }

        auto suggest = CreateSuggest(nlgTemplateName, block, nlgData);
        if (suggest.Defined()) {
            bodyBuilder.AddRenderedSuggest(std::move(suggest.GetRef()));
        }
    }

    LOG_INFO(Logger_) << "Rendering post-error server directives and other uniproxy-action commands";
    for (const auto* blockPtr : FilterBlocksByType(blocks, "uniproxy-action")) {
        const auto& block = *blockPtr;
        Y_ENSURE(block.Has("command_type"));
        const auto& commandType = block["command_type"].GetString();
        const auto& data = block["data"];
        Y_ENSURE(data.IsDefined());
        if (PostErrorServerDirectiveParsers.contains(commandType) &&
            !NImpl::AddServerDirective(bodyBuilder, Logger_, *PostErrorServerDirectiveParsers.at(commandType), data)) {
            continue;
        } else if (commandType == "save_voiceprint" || commandType == "remove_voiceprint") {
            bodyBuilder.AddClientActionDirective(commandType, commandType, data);
        } else if (PreErrorServerDirectiveParsers.contains(commandType)) {
            continue;
        } else {
            LOG_INFO(Logger_) << "Do nothing with other uniproxy-action command: " << commandType;
            continue;
        }
    }

    if (errorBlocks) {
        Y_ENSURE(processSuggestsOnError, "The only allowed condition to enter here");
        // bodyBuilder is alreadry rendered.
        return;
    }

    LOG_INFO(Logger_) << "Rendering text_card blocks";
    for (const auto* blockPtr : FilterBlocksByType(blocks, "text_card")) {
        const auto& block = *blockPtr;
        const auto& phraseId = block["phrase_id"].GetStringSafe();
        TNlgData cardNlgData(nlgData);
        cardNlgData.Context["data"] = block["data"];
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(nlgTemplateName, phraseId, /* buttons = */ {}, cardNlgData);
    }

    LOG_INFO(Logger_) << "Rendering the main phrase and div/div2 cards";
    auto hasDivCard = AddRenderedDivCards(nlgTemplateName, bodyBuilder, blocks, nlgData);
    if (!hasDivCard && !BaseRequest_.HasExpFlag(NExperiments::DISABLE_EXPERIMENTAL_FLAG_MUSIC_SEARCH_APP_NEW_CARDS)) {
        LOG_INFO(Logger_) << "No div card found, trying to find div2";
        hasDivCard = AddRenderedDiv2Cards(nlgTemplateName, bodyBuilder, blocks, nlgData);
    }
    if (hasDivCard) {
        LOG_INFO(Logger_) << "Trying to add Voice";
        // the phrase is used for voicing the card response, no text bubble needed
        bodyBuilder.TryAddRenderedVoice(nlgTemplateName, nlgPhraseName, nlgData);
    } else {
        // no div card, so the phrase is used both for TTS and the speech bubble
        const auto& phraseName = IsPlayerFormFromMusicCommands(bassResponse) ? NLG_PLAYER_NAME : nlgPhraseName;
        bodyBuilder.TryAddRenderedTextWithButtonsAndVoice(nlgTemplateName, phraseName, /* buttons = */ {}, nlgData);
    }

    LOG_INFO(Logger_) << "Filling analytics info";
    analyticsInfoBuilder.FillAnalyticsInfo(analyticsIntentName, analyticsScenarioName);

    // TODO(a-square): modularize this sort analytics info construction
    if (AddFirstTrackObject_ && (firstTrackId || firstTrackGenre || firstTrackDuration || firstTrackAlbumType)) {
        NScenarios::TAnalyticsInfo::TObject firstTrackObject;
        firstTrackObject.SetId("music.first_track_id");
        firstTrackObject.SetName("first_track_id");
        firstTrackObject.SetHumanReadable(firstTrackHumanReadable);
        firstTrackObject.MutableFirstTrack()->SetId(firstTrackId);
        firstTrackObject.MutableFirstTrack()->SetGenre(firstTrackGenre);
        firstTrackObject.MutableFirstTrack()->SetDuration(firstTrackDuration);
        firstTrackObject.MutableFirstTrack()->SetAlbumType(firstTrackAlbumType);
        analyticsInfoBuilder->AddObject(firstTrackObject);
    }
}

bool TBassResponseRenderer::AddRenderedDivCards(const TStringBuf nlgTemplateName,
                                                TResponseBodyBuilder& builder,
                                                const NJson::TJsonValue::TArray& bassResponseBlocks,
                                                const TNlgData& baseNlgData) {
    bool hasDivCard = false;
    for (const auto* blockPtr : FilterBlocksByType(bassResponseBlocks, "div_card")) {
        const auto& block = *blockPtr;
        hasDivCard = true;
        const auto& cardId = block["card_template"].GetStringSafe();

        TNlgData nlgData(baseNlgData);
        nlgData.Context["data"] = block["data"];
        builder.AddRenderedDivCard(nlgTemplateName, cardId, nlgData, ReduceWhitespaceInCards_);
    }
    return hasDivCard;
}

bool TBassResponseRenderer::AddRenderedDiv2Cards(const TStringBuf nlgTemplateName,
                                                 TResponseBodyBuilder& builder,
                                                 const NJson::TJsonValue::TArray& bassResponseBlocks,
                                                 const TNlgData& baseNlgData) {
    bool hasDiv2Card = false;
    for (const auto* blockPtr : FilterBlocksByType(bassResponseBlocks, "div2_card")) {
        const auto& block = *blockPtr;
        hasDiv2Card = true;
        const auto& cardId = block["card_template"].GetStringSafe();

        TNlgData nlgData(baseNlgData);
        nlgData.Context["data"] = block["data"];
        builder.AddRenderedDiv2Card(nlgTemplateName, cardId, nlgData, ReduceWhitespaceInCards_);
    }
    return hasDiv2Card;
}

TMaybe<TResponseBodyBuilder::TSuggest> TBassResponseRenderer::CreateSuggest(const TStringBuf nlgTemplateName,
                                                                            const NJson::TJsonValue& suggestBlock,
                                                                            const TNlgData& baseNlgData) {
    Y_ENSURE(suggestBlock.Has("suggest_type"), "block must have `suggest_type` field");
    const auto& suggestType = suggestBlock["suggest_type"].GetStringSafe();

    if (suggestBlock.Has(TStringBuf("form_update"))) {
        LOG_WARNING(Logger_) << "Attempting to render a suggest with form_update present, it will be ignored: " << suggestBlock;
    }

    return CreateSuggest(Builder_.GetNlgWrapper(),
                         nlgTemplateName,
                         suggestType,
                         /* analyticsTypeAction= */ "render_buttons",
                         /* autoAction= */ SuggestAutoAction_,
                         suggestBlock["data"],
                         baseNlgData,
                         &BaseRequest_.Interfaces());
}

TMaybe<TResponseBodyBuilder::TSuggest> TBassResponseRenderer::CreateSuggest(TNlgWrapper& nlg,
                                                                            const TStringBuf nlgTemplateName,
                                                                            const TStringBuf type,
                                                                            const TStringBuf analyticsTypeAction,
                                                                            bool autoAction,
                                                                            const NJson::TJsonValue& data,
                                                                            const TNlgData& baseNlgData,
                                                                            const NAlice::NScenarios::TInterfaces* interfaces,
                                                                            const TMaybe<TStringBuf> screenId)
{
    TResponseBodyBuilder::TSuggest result;

    const TString renderCaptionPhraseId(TStringBuilder() << TStringBuf("render_suggest_caption__") << type);
    const TString renderUtterancePhraseId(TStringBuilder() << TStringBuf("render_suggest_utterance__") << type);
    const TString renderUserUtterancePhraseId(TStringBuilder() << TStringBuf("render_suggest_user_utterance__") << type);
    const TString renderUriPhraseId(TStringBuilder() << TStringBuf("render_suggest_uri__") << type);

    const bool hasCaption = nlg.HasPhrase(nlgTemplateName, renderCaptionPhraseId);
    const bool hasUtterance = nlg.HasPhrase(nlgTemplateName, renderUtterancePhraseId);
    const bool hasUserUtterance = nlg.HasPhrase(nlgTemplateName, renderUserUtterancePhraseId);
    const bool hasUri = nlg.HasPhrase(nlgTemplateName, renderUriPhraseId);
    // TODO(tolyandex) Add directives with commands

    if (!hasCaption) {
        return Nothing();
    }

    TNlgData nlgData(baseNlgData);
    nlgData.Context[type]["data"] = data;

    auto caption = nlg.RenderPhrase(nlgTemplateName, renderCaptionPhraseId, nlgData).Text;

    if (hasUserUtterance) {
        const auto userUtterance = nlg.RenderPhrase(nlgTemplateName, renderUserUtterancePhraseId, nlgData).Text;
        if (userUtterance) {
            const TString analyticsType = analyticsTypeAction ? Join("_", analyticsTypeAction, "type_silent") : "type_silent";
            auto action = CreateTypeTextSilentDirective(analyticsType, userUtterance);
            result.Directives.push_back(std::move(action));
        }
    }

    if (hasUri) {
        const auto uri = nlg.RenderPhrase(nlgTemplateName, renderUriPhraseId, nlgData).Text;
        if (uri) {
            const TString analyticsType = analyticsTypeAction ? Join("_", analyticsTypeAction, "open_uri") : "open_uri";
            auto action = CreateOpenUriDirective(analyticsType, uri, screenId);
            result.Directives.push_back(action);
            if (autoAction) {
                result.AutoDirective = std::move(action);
            }
        }
    }

    if (hasUtterance) {
        const auto utterance = nlg.RenderPhrase(nlgTemplateName, renderUtterancePhraseId, nlgData).Text;
        if (utterance) {
            const TString analyticsType = analyticsTypeAction ? Join("_", analyticsTypeAction, "type") : "type";
            auto action = CreateTypeTextDirective(analyticsType, utterance);
            result.Directives.push_back(std::move(action));
        }
    }

    bool isExternal = type == TStringBuf("external_skill");

    bool isSuggest;
    if (isExternal) {
        isSuggest = data["hide"].GetBooleanSafe(true);
    } else {
        if (!data.IsNull() && data.Has("attach_to_card")) {
            isSuggest = !data["attach_to_card"].GetBoolean();
        } else {
            isSuggest = !hasUri;
        }
    }

    if (interfaces && !interfaces->GetSupportsButtons()) {
        isSuggest = true;
    }

    if (isSuggest) {
        result.SuggestButton = std::move(caption);
    } else {
        result.ButtonForText = std::move(caption);
    }

    return std::move(result);
}

void TBassResponseRenderer::SetContextValue(TStringBuf name, NJson::TJsonValue value) {
    NlgData_.Context[name] = std::move(value);
}

TNlgData TBassResponseRenderer::CreateNlgData() const {
    LOG_INFO(Logger_) << "Creating NLG data";
    TNlgData nlgData{Logger_, BaseRequest_};
    EnrichNlgData(BaseRequest_, Input_, nlgData);
    return nlgData;
}

} // namespace NAlice::NHollywood
