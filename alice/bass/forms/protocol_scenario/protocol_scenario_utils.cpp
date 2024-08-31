#include "protocol_scenario_utils.h"

#include <alice/bass/libs/video_common/parsers/video_item.h>
#include <alice/bass/common_nlg/register.h>
#include <alice/library/json/json.h>
#include <alice/library/response/defs.h>
#include <alice/library/video_common/defs.h>
#include <alice/megamind/protos/common/app_type.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/push.pb.h>
#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_register_function.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/protos/endpoint/capability.pb.h>

#include <google/protobuf/struct.pb.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/protobuf/json/proto2json.h>

namespace NBASS {

using namespace NAlice;
using namespace NAlice::NScenarios;
using namespace NAlice::NVideoCommon;

namespace {

constexpr TStringBuf ATTENTIONS_KEY = "attentions";
constexpr TStringBuf OPEN_URI_KEY = "commands_open_uri";

struct TCommand {
    TString Type;
    TString SubType;
    NJson::TJsonValue Payload;
};

struct TNlgPhrase {
    TString TemplateId;
    TString PhraseId;
    NNlg::TRenderContextData RenderContextData;
};

class TProtocolResponseBuilder {
public:
    TProtocolResponseBuilder(const TMaybe<TString>& nlgTemplateId, IRng& rng, ELanguage lang)
        : Rng{rng}
        , Lang{lang}
    {
        if (nlgTemplateId) {
            NlgPhrase = TNlgPhrase {
                .TemplateId = nlgTemplateId.GetRef(),
                .PhraseId = "render_result",
            };
            NlgPhrase->RenderContextData.Context[ATTENTIONS_KEY].SetType(NJson::JSON_MAP);
        }

        SetShouldListen(true);
    }

    void AddCommand(TStringBuf commandType, TStringBuf commandSubType, const NSc::TValue& payload) {
        NJson::TJsonValue payloadJson = payload.ToJsonValue();
        Commands.emplace_back(TCommand{TString{commandType}, TString{commandSubType}, payloadJson});
        if (!payload[NResponse::LISTENING_IS_POSSIBLE].GetBool(false)) {
            SetShouldListen(false);
        }
    }

    void AddAttention(TStringBuf type, const NSc::TValue& payload) {
        if (NlgPhrase.Defined()) {
            NlgPhrase->RenderContextData.Context[ATTENTIONS_KEY][type] = payload.ToJsonValue();
        }
    }

    void AddOpenUriCommand(const NSc::TValue& block) {
        if (NlgPhrase.Defined()) {
            NlgPhrase->RenderContextData.Context[OPEN_URI_KEY].AppendValue(block.ToJsonValue());
        }
    }

    void SetForm(const NSc::TValue& form) {
        if (NlgPhrase.Defined()) {
            NlgPhrase->RenderContextData.Form = form.ToJsonValue();
        }
    }

    void SetFormName(const NSc::TValue& formName) {
        if (formName.IsString()) {
            FormName = formName.GetString();
        }
    }

    void AddSimpleText(const TString& text, const TString& tts) {
        if (tts) {
            *Proto.MutableLayout()->MutableOutputSpeech() = tts;
        }
        auto& card = *Proto.MutableLayout()->AddCards();
        card.SetText(text);
    }

    void SetAnalyticsInfo(const TAnalyticsInfo& analyticsInfoProto) {
        AnalyticsInfoProto = analyticsInfoProto;
    }

    void SetShouldListen(bool shouldListen) {
        Proto.MutableLayout()->SetShouldListen(shouldListen);
    }

    TErrorOr<TScenarioResponseBody> Build();

private:
    // Usually, we don't have a lot of attentions and blocks, so 4 is even too much in most cases.
    template <typename T>
    using TSmallVec = TStackVec<T, 4 /* CountOnStack */, true /* UseFallbackAlloc */>;

    TSmallVec<TCommand> Commands;
    TSmallVec<NJson::TJsonValue> CommandsOpenUri;
    TSmallVec<std::pair<TString, NJson::TJsonValue>> Attentions;

    TMaybe<TNlgPhrase> NlgPhrase;
    IRng& Rng;
    ELanguage Lang;
    TMaybe<TString> FormName;

    TScenarioResponseBody Proto;
    TAnalyticsInfo AnalyticsInfoProto;
};

TDirective CreateOpenUriDirective(const TCommand& command) {
    TDirective oneOfDirective;
    TOpenUriDirective directive;
    directive.SetName(command.Type);
    directive.SetUri(command.Payload["uri"].GetString());
    *oneOfDirective.MutableOpenUriDirective() = std::move(directive);
    return oneOfDirective;
}

TDirective CreateMordoviaShowDirective(const TCommand& command) {
    TDirective oneOfDirective;
    TMordoviaShowDirective& directive = *oneOfDirective.MutableMordoviaShowDirective();

    directive.SetName(command.Type);
    directive.SetUrl(command.Payload["url"].GetString());
    directive.SetViewKey(command.Payload["view_key"].GetString());
    directive.SetSplashDiv(command.Payload["splash_div"].GetString());
    directive.SetGoBack(command.Payload["go_back"].GetBoolean());

    TCallbackDirective& callback = *directive.MutableCallbackPrototype();
    callback.SetName(command.Payload["callback_name"].GetString());

    return oneOfDirective;
}

TDirective CreateMordoviaCommandDirective(const TCommand& command) {
    TDirective oneOfDirective;
    TMordoviaCommandDirective& directive = *oneOfDirective.MutableMordoviaCommandDirective();

    directive.SetName(command.Type);
    directive.SetCommand(command.Payload["command"].GetString());
    directive.SetViewKey(command.Payload["view_key"].GetString());

    auto mutableFields = directive.MutableMeta()->mutable_fields();
    for (const auto& field : command.Payload["meta"].GetMap()) {
        if (field.second.IsString()) {
            (*mutableFields)[field.first].set_string_value(field.second.GetString());
        } else if (field.second.IsBoolean()) {
            (*mutableFields)[field.first].set_bool_value(field.second.GetBoolean());
        }
    }

    return oneOfDirective;
}

TDirective CreateWebOSLaunchAppDirective(const TCommand& command) {
    TDirective oneOfDirective;
    TWebOSCapability_TWebOSLaunchAppDirective directive;
    directive.SetName(command.Type);
    directive.SetAppId(command.Payload["appId"].GetString());
    directive.SetParamsJson(JsonToString(command.Payload["params"]));
    *oneOfDirective.MutableWebOSLaunchAppDirective() = std::move(directive);
    return oneOfDirective;
}

TDirective CreateWebOSShowGalleryDirective(const TCommand& command) {
    TDirective oneOfDirective;
    TWebOSCapability_TWebOSShowGalleryDirective directive;
    directive.SetName(command.Type);
    for (const auto& itemJson : command.Payload["itemsJson"].GetArray()) {
        auto& item = *directive.AddItemsJson();
        item = JsonToString(itemJson);
    }
    *oneOfDirective.MutableWebOSShowGalleryDirective() = std::move(directive);
    return oneOfDirective;
}

void FillCommonSettings(TSendPushDirective::TCommon& commonSettings, const NJson::TJsonValue& payload, const TString& prefix = "") {
    NJson::TJsonValue paramValue;
    auto getParam = [&payload, &prefix, &paramValue](const TString& param) {
        paramValue = payload[(prefix.empty() ? "" : prefix + "_") + param];
        return !paramValue.IsNull();
    };
    if (getParam("title"))  commonSettings.SetTitle(paramValue.GetString());
    if (getParam("text"))   commonSettings.SetText(paramValue.GetString());
    if (getParam("url"))    commonSettings.SetLink(paramValue.GetString());
    if (getParam("ttl"))    commonSettings.SetTtlSeconds(paramValue.GetUInteger());
}

TServerDirective CreateSendPushDirective(const TCommand& command) {
    auto getStringParam = [&command](const TString& param) {
        return command.Payload[param].GetString();
    };
    TServerDirective oneOfDirective;
    TSendPushDirective& directive = *oneOfDirective.MutableSendPushDirective();

    // common settings
    TSendPushDirective::TCommon& commonSettings = *directive.MutableSettings();
    FillCommonSettings(commonSettings, command.Payload);
    // push message
    TSendPushDirective::TPush& pushMessage = *directive.MutablePushMessage();
    pushMessage.SetThrottlePolicy(getStringParam("throttle"));
    pushMessage.AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);
    TSendPushDirective::TCommon& commonSettingsPush = *pushMessage.MutableSettings();
    FillCommonSettings(commonSettingsPush, command.Payload, "push");
    // personal card
    TSendPushDirective::TPersonalCard& personalCard = *directive.MutablePersonalCard();
    personalCard.SetImageUrl(getStringParam("personal_card_image_url"));
    if (command.Payload.Has("min_price")) {
        auto& cardData = *personalCard.MutableYandexStationFilmData();
        cardData.SetMinPrice(command.Payload["min_price"].GetUInteger());
    }
    TSendPushDirective::TCommon& commonSettingsPersonalCard = *personalCard.MutableSettings();
    FillCommonSettings(commonSettingsPersonalCard, command.Payload, "personal_card");
    // misc
    directive.SetPushId(getStringParam("id"));
    directive.SetPushTag(getStringParam("tag"));
    directive.SetRemoveExistingCards(command.Payload["remove_existing_cards"].GetBoolean());
    if (command.Payload.Has("actions")) {
        for (const auto& action: command.Payload["actions"].GetArray()) {
            TSendPushDirective::TAction& tAction = *directive.AddActions();
            tAction.SetId(action["id"].GetString());
            tAction.SetTitle(action["title"].GetString());
        }
    }

    return oneOfDirective;
}

TServerDirective CreatePersonalCardsDirective(const TCommand& command) {
    TServerDirective oneOfDirective;
    TPersonalCardsDirective& directive = *oneOfDirective.MutablePersonalCardsDirective();

    auto& card = *directive.MutableCard();
    auto& payloadCard = command.Payload["card"];
    card.SetCardId(payloadCard["card_id"].GetString());
    card.SetButtonUrl(payloadCard["button_url"].GetString());
    card.SetText(payloadCard["text"].GetString());
    card.SetDateFrom(payloadCard["date_from"].GetDouble());
    card.SetDateTo(payloadCard["date_to"].GetDouble());

    if (payloadCard.Has("yandex.station_film")) {
        if (payloadCard["yandex.station_film"].Has("min_price")) {
            auto& cardData = *card.MutableYandexStationFilmData();
            cardData.SetMinPrice(payloadCard["yandex.station_film"]["min_price"].GetUInteger());
        }
    }

    directive.SetRemoveExistingCards(command.Payload["remove_existing_cards"].GetBoolean());

    return oneOfDirective;
}

TServerDirective CreatePushMessageDirective(const TCommand& command) {
    TServerDirective oneOfDirective;
    TPushMessageDirective& directive = *oneOfDirective.MutablePushMessageDirective();
    auto& payload = command.Payload;

    directive.SetTitle(payload["title"].GetString());
    directive.SetBody(payload["text"].GetString());
    directive.SetLink(payload["url"].GetString());
    directive.SetPushId(payload["id"].GetString());
    directive.SetPushTag(payload["tag"].GetString());
    directive.SetThrottlePolicy(payload["throttle"].GetString());
    directive.AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);
    return oneOfDirective;
}

#if defined(CREATE_DIRECTIVE_FROM_PAYLOAD)
#error CREATE_DIRECTIVE_FROM_PAYLOAD should not be defined at this point
#endif

#define CREATE_DIRECTIVE_FROM_PAYLOAD(directiveType)                                                                  \
    [](const TCommand& command) {                                                                                     \
        TDirective oneOfDirective;                                                                                    \
        auto directive =                                                                                              \
            JsonToProto<T##directiveType>(command.Payload, false /* validateUTF8 */, true /* ignoreUnknownFields */); \
        directive.SetName(command.SubType);                                                                           \
        *oneOfDirective.Mutable##directiveType() = std::move(directive);                                              \
        return oneOfDirective;                                                                                        \
    }

TErrorOr<TScenarioResponseBody> TProtocolResponseBuilder::Build() {
    if (NlgPhrase) {
        const auto nlg = NNlg::CreateNlgRendererFromRegisterFunction(NAlice::NBass::NCommonNlg::RegisterAll, Rng);
        try {
            const auto phrase = nlg->RenderPhrase(
                NlgPhrase->TemplateId, NlgPhrase->PhraseId, Lang, Rng, NlgPhrase->RenderContextData);
            this->AddSimpleText(phrase.Text, phrase.Voice);
        } catch (...) {
            return ::NAlice::TError{::NAlice::TError::EType::NLG} << "Unable to render a phrase: " << CurrentExceptionMessage();
        }
    }

    const THashMap<TStringBuf, std::function<TDirective(const TCommand&)>> directiveFactory = {
        {COMMAND_CHANGE_AUDIO, CREATE_DIRECTIVE_FROM_PAYLOAD(ChangeAudioDirective)},
        {COMMAND_CHANGE_SUBTITLES, CREATE_DIRECTIVE_FROM_PAYLOAD(ChangeSubtitlesDirective)},
        {COMMAND_PLAYER_REWIND, CREATE_DIRECTIVE_FROM_PAYLOAD(PlayerRewindDirective)},
        {COMMAND_SHOW_DESCRIPTION, CREATE_DIRECTIVE_FROM_PAYLOAD(ShowVideoDescriptionDirective)},
        {COMMAND_SHOW_GALLERY, CREATE_DIRECTIVE_FROM_PAYLOAD(ShowGalleryDirective)},
        {COMMAND_SHOW_SEASON_GALLERY, CREATE_DIRECTIVE_FROM_PAYLOAD(ShowSeasonGalleryDirective)},
        {COMMAND_SHOW_PAY_PUSH_SCREEN, CREATE_DIRECTIVE_FROM_PAYLOAD(ShowPayPushScreenDirective)},
        {COMMAND_SHOW_VIDEO_SETTINGS, CREATE_DIRECTIVE_FROM_PAYLOAD(ShowVideoSettingsDirective)},
        {COMMAND_TTS_PLAY_PLACEHOLDER, CREATE_DIRECTIVE_FROM_PAYLOAD(TtsPlayPlaceholderDirective)},
        {COMMAND_VIDEO_PLAY, CREATE_DIRECTIVE_FROM_PAYLOAD(VideoPlayDirective)},
        {COMMAND_GO_BACKWARD, CREATE_DIRECTIVE_FROM_PAYLOAD(GoBackwardDirective)},
        {COMMAND_TV_OPEN_SEARCH_SCREEN, CREATE_DIRECTIVE_FROM_PAYLOAD(TvOpenSearchScreenDirective)},
        {COMMAND_TV_OPEN_DETAILS_SCREEN, CREATE_DIRECTIVE_FROM_PAYLOAD(TvOpenDetailsScreenDirective)},
        {COMMAND_TV_OPEN_PERSON_SCREEN, CREATE_DIRECTIVE_FROM_PAYLOAD(TvOpenPersonScreenDirective)},
        {COMMAND_TV_OPEN_COLLECTION_SCREEN, CREATE_DIRECTIVE_FROM_PAYLOAD(TvOpenCollectionScreenDirective)},
        {COMMAND_TV_OPEN_SERIES_SCREEN, CREATE_DIRECTIVE_FROM_PAYLOAD(TvOpenSeriesScreenDirective)},
        {COMMAND_MORDOVIA_SHOW, CreateMordoviaShowDirective},
        {COMMAND_MORDOVIA_COMMAND, CreateMordoviaCommandDirective},
        {COMMAND_OPEN_URI, CreateOpenUriDirective},
        {COMMAND_WEB_OS_LAUNCH_APP, CreateWebOSLaunchAppDirective},
        {COMMAND_WEB_OS_SHOW_GALLERY, CreateWebOSShowGalleryDirective},
    };

    const THashMap<TStringBuf, std::function<TServerDirective(const TCommand&)>> serverDirectiveFatory = {
        {COMMAND_SEND_PUSH, CreateSendPushDirective},
        // deprecated
        {COMMAND_PERSONAL_CARDS, CreatePersonalCardsDirective},
        {COMMAND_PUSH_MESSAGE, CreatePushMessageDirective}
    };

    for (const TCommand& command : Commands) {
        const auto* directiveHandler = directiveFactory.FindPtr(command.Type);
        const auto* serverDirectiveHandler = serverDirectiveFatory.FindPtr(command.Type);
        if (directiveHandler) {
            *Proto.MutableLayout()->AddDirectives() = (*directiveHandler)(command);
        } else if (serverDirectiveHandler) {
            *Proto.AddServerDirectives() = (*serverDirectiveHandler)(command);
        } else {
            return NAlice::TError{NAlice::TError::EType::ScenarioError} << "Unknown directive type: " << command.Type;
        }
    }

    *Proto.MutableAnalyticsInfo() = AnalyticsInfoProto;
    if (FormName.Defined()) {
        Proto.MutableAnalyticsInfo()->SetIntent("mm." + *FormName);
    }

    return Proto;
}

#undef CREATE_DIRECTIVE_FROM_PAYLOAD

bool IsSilentResponse(const TContext& ctx) {
    if (ctx.HasAnyBlockOfType(NResponse::TYPE_SILENT_RESPONSE)) {
        return true;
    }
    return false;
}

} // namespace

TErrorOr<TScenarioResponseBody> BassContextToProtocolResponseBody(TContext& ctx,
                                                                  const TMaybe<TString>& originNlgTemplateId) {
    const TMaybe<TString> nlgTemplateId = IsSilentResponse(ctx) ? Nothing() : originNlgTemplateId;
    TProtocolResponseBuilder responseBuilder{nlgTemplateId, ctx.GetRng(), LanguageByName(ctx.MetaLocale().Lang)};
    NSc::TValue serializedCtx;
    if (nlgTemplateId) {
        serializedCtx = ctx.ToJson(); // FIXME (a-sidorin@): Make a separate method for form serialization.
    }

    // Applying the same logic to the shouldListen block as in
    // alice/vins/apps/personal_assistant/personal_assistant/app.py
    if (ctx.HasAnyBlockOfType(NResponse::TYPE_STOP_LISTENING)) {
        responseBuilder.SetShouldListen(false);
    }
    if (ctx.HasAnyBlockOfType(NResponse::TYPE_SILENT_RESPONSE)) {
        responseBuilder.SetShouldListen(false);
    }

    for (const auto blockRef : ctx.GetBlocks()) {
        const auto& block = blockRef.get();
        const auto& blockType = block["type"];
        if (!blockType.IsString()) {
            continue;
        }

        if (blockType == NResponse::TYPE_COMMAND) {
            const auto& commandType = block["command_type"].GetString();
            responseBuilder.AddCommand(commandType, block["command_sub_type"].GetString(), block["data"]);
            if (commandType == COMMAND_OPEN_URI) {
                responseBuilder.AddOpenUriCommand(block);
            }

        } else if (blockType == NResponse::TYPE_ATTENTION) {
            responseBuilder.AddAttention(block["attention_type"].GetString(), block["data"]);
        }
    }

    responseBuilder.SetAnalyticsInfo(ctx.GetAnalyticsInfoBuilder().Build());

    const auto& form = serializedCtx["form"];

    if (nlgTemplateId) {
        responseBuilder.SetForm(form);
    }

    responseBuilder.SetFormName(form["name"]);

    return responseBuilder.Build();
}

} // namespace NBASS
