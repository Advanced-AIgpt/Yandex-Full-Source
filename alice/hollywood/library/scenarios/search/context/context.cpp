#include "context.h"

#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/framework/framework_migration.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/request/utils/nlu_features.h>
#include <alice/hollywood/library/scenarios/search/scenarios/ellipsis_intents.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/url_builder/url_builder.h>
#include <alice/megamind/protos/analytics/scenarios/search/search.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>
#include <alice/protos/data/language/language.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/search/fact.pb.h>
#include <alice/protos/data/scenario/search/search_object.pb.h>

namespace NAlice::NHollywood::NSearch {

namespace {

constexpr TStringBuf SEE_ALSO = "search__see_also";
constexpr TStringBuf SCREEN_ID_CLOUD_UI = "cloud_ui";

bool CanAddCloudUiDirective(const TScenarioRunRequestWrapper& request) {
    return request.Proto().GetBaseRequest().GetInterfaces().GetSupportsCloudUi()
        && request.HasExpFlag(EXP_SEARCH_USE_CLOUD_UI);
}

bool TryAddCloudUiDirective(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder, const TString& uri) {
    if (!CanAddCloudUiDirective(request)) {
        return false;
    }
    bodyBuilder.AddOpenUriDirective(uri, SCREEN_ID_CLOUD_UI);
    return true;
}

} // namespace

const TSearchScenarioResources& TSearchContext::GetResources() const {
    return Ctx.ScenarioResources<TSearchScenarioResources>();
}

const TScenarioRunRequestWrapper& TSearchContext::GetRequest() const {
    return RunRequestWrapper;
}

NScenarios::IAnalyticsInfoBuilder& TSearchContext::GetAnalyticsInfoBuilder() {
    return BodyBuilder.GetAnalyticsInfoBuilder();
}

TRTLogger& TSearchContext::GetLogger() {
    return Ctx.Logger();
}

TStringBuf TSearchContext::GetLangName() const {
    return LangName;
}

const NScenarios::TRequestMeta& TSearchContext::GetRequestMeta() const {
    return RequestMeta;
}

bool TSearchContext::IsPornoQuery() const {
    const TMaybe<float> pornQuery = GetNluFeatureValue(RunRequestWrapper, NNluFeatures::ENluFeature::IsPornQuery);
    return pornQuery.Defined() && pornQuery.GetRef() > 0;
}

bool TSearchContext::IsRecipeQuery() const {
    for (const auto& frame: RunRequestWrapper.Input().Proto().GetSemanticFrames()) {
        if (frame.GetName() == "alice.recipes.select_recipe") {
            return true;
        }
    }
    return false;
}

bool TSearchContext::IsSerpSupported() const {
    const auto& client = GetRequest().ClientInfo();
    return !(client.IsSmartSpeaker() || client.IsElariWatch() || client.IsYaAuto() || client.IsTvDevice() || client.IsLegatus());
}

bool TSearchContext::IsSuggestSupported() const {
    const auto& client = GetRequest().ClientInfo();
    return !(client.IsQuasar() || client.IsMiniSpeaker() || client.IsElariWatch() || client.IsYaAuto() || client.IsTvDevice() || client.IsLegatus());
}

void TSearchContext::SetResultSlot(const TStringBuf type, const NJson::TJsonValue& data, bool setIntent) {
    LOG_INFO(Ctx.Logger()) << "Adding result slot: " << type;
    LOG_INFO(Ctx.Logger()) << "Result slot data: " << data;
    // deprecated
    GetAnalyticsInfoBuilder().AddObject("selected_fact", JsonToString(data), "Дамп выбранного сценарием факта");
    AddTypedSelectedFactAnalytics("typed_selected_fact", "typed_selected_fact", "Дамп выбранного сценарием факта", data);
    if (NlgData.Form["slots"].GetArray().size() > 0) {
        NlgData.Form["slots"][0]["value"][type] = data;
        const TString typeStr{type};
        if (setIntent) {
            SetIntent(typeStr);
        }
        if (data.Has("source")) {
            SetFactoidSource(data["source"].GetString());
        } else {
            SetFactoidSource(typeStr);
        }
    } else {
        LOG_DEBUG(Ctx.Logger()) << "Failed search scenario init!";
        Builder.SetIrrelevant();
    }
}

void TSearchContext::AddTypedSelectedFactAnalytics(const TString& id, const TString& name, const TString& description, const NJson::TJsonValue& fact) {
    NScenarios::TAnalyticsInfo::TObject object;
    object.SetId(id);
    object.SetName(name);
    object.SetHumanReadable(description);

    NAlice::NSearch::TSearchFact selectedFact;
    if (fact.IsString()) {
        selectedFact.SetText(fact.GetString());
    } else if (const auto status = JsonToProto(fact, selectedFact, /* validateUtf8 */ true, /* ignoreUnknownFields */ true); !status.ok()) {
        LOG_ERROR(GetLogger()) << "Failed to convert selected fact to proto";
        return;
    }
    *object.MutableSearchFact() = selectedFact;

    GetAnalyticsInfoBuilder().AddObject(object);
}

void TSearchContext::AddAttention(const TStringBuf attention) {
    NlgData.Context["attentions"][attention] = true;
}

void TSearchContext::AddRenderedCard(NJson::TJsonValue& cardData, const TStringBuf slot, const TStringBuf type) {
    if (cardData["search_url"].GetString().empty()) {
        cardData["search_url"] = GenerateSearchUri();
    }

    NlgData.Context["data"] = cardData;
    SetResultSlot(slot, cardData);
    if (RunRequestWrapper.Interfaces().GetCanRenderDivCards()) {
        LOG_INFO(Ctx.Logger()) << "Rendering factoid div card";
        BodyBuilder.AddRenderedDivCard("search_factoid_div_cards", type, NlgData);
        BodyBuilder.AddRenderedVoice("search", "render_result", NlgData);
        ShouldOpenLink = true;
        ForbidText = true;
    } else if (RunRequestWrapper.Interfaces().GetCanOpenLink()) {
        ShouldOpenLink = true;
    }
}

void TSearchContext::AddSuggest(const TStringBuf type, bool autoAction, const NJson::TJsonValue& data) {
    Suggests.push_back({.Type = type, .AutoAction = autoAction, .Data = data});
}

void TSearchContext::RenderSuggest(const TSuggest suggest) {
    auto renderedSuggest = TBassResponseRenderer::CreateSuggest(Nlg, TStringBuf("search"), suggest.Type,
                                                        /* analyticsTypeAction = */ "", suggest.AutoAction,
                                                        suggest.Data, NlgData, &GetRequest().Interfaces());
    if (renderedSuggest.Defined()) {
        BodyBuilder.AddRenderedSuggest(std::move(renderedSuggest.GetRef()));
    }
}

void TSearchContext::AddRelatedSuggest(const TString& text) {
    NJson::TJsonValue rel;
    rel["query"] = text;
    GetNlgData().Context[SEE_ALSO]["data"]["query"] = text;
    AddSuggest(SEE_ALSO, /* autoplay */ false, rel);
}

void TSearchContext::AddResult() {
    State.SetPreviousQuery(GetQuery());
    LOG_INFO(Ctx.Logger()) << "Set PreviousQuery " << GetQuery();
    if ((GetRequest().ClientInfo().IsSmartSpeaker() || GetRequest().ClientInfo().IsTvDevice() || GetRequest().ClientInfo().IsLegatus())) {
        if (RunRequestWrapper.HasExpFlag(NExperiments::FACTOID_RECIPE_PREROLL) && IsRecipeQuery()) {
            AddAttention(TStringBuf("factoid_recipe_preroll"));
        }
        AddPushButton(*this);
        if (!HasPostroll) {
            AddPushPromo();
            State.SetLastReplyHadHandoffPromo(true);
        } else {
            State.SetLastReplyHadHandoffPromo(false);
        }
    }

    BodyBuilder.SetShouldListen(ShouldListen);
    for (const auto& suggest : Suggests) {
        RenderSuggest(suggest);
    }

    if (!ForbidText) {
        LOG_INFO(Ctx.Logger()) << "Rendering text with buttons";
        BodyBuilder.AddRenderedTextWithButtonsAndVoice("search", "render_result", /* buttons = */ {}, NlgData);
    }

    if (ShouldOpenLink) {
        if (TryAddCloudUiDirective(GetRequest(), GetBodyBuilder(), GenerateSearchUri())) {
            LOG_INFO(Ctx.Logger()) << "Add open_uri directive with cloud_ui";
        }
    }

    BodyBuilder.SetIsResponseConjugated(true);
}

void TSearchContext::AddPushPromoButton(const TString& attentionName) {
    AddAttention(attentionName);
    AddPushButton(*this, "alice.search.related_agree");
    HasPostroll = true;
    GetAnalyticsInfoBuilder().AddObject("handoff_promo", "Handoff promo", "Промо Handoff");
    AddDoNothingButton();
}

void TSearchContext::ReturnIrrelevant(bool hardIrrelevant, bool addHandoffPromo) {
    if (hardIrrelevant) {
        Builder.SetIrrelevant();
    }
    auto handoffAllowedOnDevice = (GetRequest().ClientInfo().IsSmartSpeaker() || GetRequest().ClientInfo().IsTvDevice() || GetRequest().ClientInfo().IsLegatus());
    if (handoffAllowedOnDevice && !State.GetLastReplyHadHandoffPromo() && addHandoffPromo && GetQuery().Contains(' ') &&
        !GetRequest().HasExpFlag(NExperiments::DISABLE_HANDOFF_ON_NO_ANSWER) &&
        GetRequest().BaseRequestProto().GetUserLanguage() == L_RUS)
    {
        State.SetPreviousQuery(GetQuery());
        State.SetLastReplyHadHandoffPromo(true);
        AddPushButton(*this);
        AddPushPromoButton("search__nothing_found_with_handoff");
        BodyBuilder.SetShouldListen(true);
    } else {
        State.SetLastReplyHadHandoffPromo(false);
    }
    AddAttention(TStringBuf("search__nothing_found"));
    BodyBuilder.GetAnalyticsInfoBuilder().AddObject("nothing_found", "true", "Поисковый сценарий не нашел подходящего ответа");
    BodyBuilder.AddRenderedTextWithButtonsAndVoice("search", "render_result", /* buttons = */ {}, NlgData);
    BodyBuilder.SetIsResponseConjugated(true);
    GetAnalyticsInfoBuilder().SetProductScenarioName("search");
}

void TSearchContext::ReturnPushNotSuccessful() {
    AddAttention(TStringBuf("search__push_not_sent"));
    BodyBuilder.AddRenderedTextWithButtonsAndVoice("search", "render_result", /* buttons = */ {}, NlgData);
}

void TSearchContext::AddAction(const TString& actionId, NScenarios::TFrameAction&& action) {
    BodyBuilder.AddAction(actionId, std::move(action));
}

NNlg::TRenderPhraseResult TSearchContext::RenderPhrase(const TStringBuf phraseName, const TStringBuf templateName) {
    return Nlg.RenderPhrase(templateName, phraseName, NlgData);
}

TString TSearchContext::GenerateSearchUri() const {
    return GenerateSearchUri(GetQuery());
}

TString TSearchContext::GenerateSearchUri(const TStringBuf query, TCgiParameters cgi) const {
    return ::NAlice::GenerateSearchUri(RunRequestWrapper.ClientInfo(), GetUserLocation(RunRequestWrapper),
                                       RunRequestWrapper.ContentRestrictionLevel(), query,
                                       RunRequestWrapper.Interfaces().GetCanOpenLinkSearchViewport(), cgi);
}

TString TSearchContext::GetTaggerQuery() const {
    constexpr TStringBuf searchFrameName = "personal_assistant.scenarios.search";
    TMaybe<TFrame> frame = GetCallbackFrame(RunRequestWrapper.Input().GetCallback());
    if (!frame.Defined() || frame->Name() != searchFrameName) {
        if (const auto frameProto = RunRequestWrapper.Input().FindSemanticFrame(searchFrameName)) {
            frame = TFrame::FromProto(*frameProto);
        }
    }
    if (frame.Defined() && frame->FindSlot("query")) {
        return frame->FindSlot("query")->Value.AsString();
    }
    return TString{};
}

TString TSearchContext::GetQuery() const {
    TString text;
    if (const auto* meta = RunRequestWrapper.GetDataSource(EDataSourceType::WEB_SEARCH_REQUEST_META)) {
        text = meta->GetWebSearchRequestMeta().GetQuery();
    } else {
        text = GetTaggerQuery();
    }

    if (!text.empty()) {
        LOG_DEBUG(Ctx.Logger()) << "Using tagger result for search query: " << text;
    } else {
        text = GetRequest().Input().Utterance();
    }
    return text;
}

void TSearchContext::PrepareContextForNlg() {
    NJson::TJsonValue slot;
    slot["name"] = "search_results";
    slot["type"] = "search_results";
    slot["value"].SetType(NJson::JSON_MAP);
    NlgData.Form["slots"].AppendValue(std::move(slot));
    NlgData.Form["name"] = "personal_assistant.scenarios.search";
}

NAlice::NScenarios::TSearchFeatures& TSearchContext::GetFeatures() {
    auto& features = Builder.GetMutableFeatures();
    const bool isFeaturesCorrect = features.GetFeaturesCase() == NScenarios::TScenarioRunResponse_TFeatures::FEATURES_NOT_SET ||
                                   features.GetFeaturesCase() == NScenarios::TScenarioRunResponse_TFeatures::kSearchFeatures;
    Y_ASSERT(isFeaturesCorrect);
    if (!isFeaturesCorrect) {
        LOG_ERROR(GetLogger()) << "Features of different type was already set";
    }
    return *features.MutableSearchFeatures();
}

const NAlice::NScenarios::TInterfaces& TSearchContext::GetInterfaces() const {
    return RunRequestWrapper.Interfaces();
}

const TMaybe<TSummarizationRequest>& TSearchContext::GetSerpSummarizationRequest() const {
    return SummarizationRequest;
}

void TSearchContext::SetSummarizationRequest(TSummarizationRequest request) {
    SummarizationRequest = request;
}

const TMaybe<THttpProxyRequest>& TSearchContext::GetSerpSummarizationAsyncRequest() const {
    return SummarizationAsyncRequest;
}

void TSearchContext::SetSummarizationAsyncRequest(THttpProxyRequest request) {
    SummarizationAsyncRequest = request;
}

const TMaybe<TString>& TSearchContext::GetSerpSummarizationHostPort() const {
    return SummarizationHostPort;
}

void TSearchContext::SetSummarizationHostPort(const TStringBuf hostPort) {
    SummarizationHostPort = hostPort;
}

bool TSearchContext::GetIsUsingState() const {
    return IsUsingState;
}

void TSearchContext::SetIsUsingState(bool value) {
    IsUsingState = value;
}

void TSearchContext::SetFactoidSource(const TString& source) {
    BodyBuilder.GetAnalyticsInfoBuilder().AddObject("factoid_src", source, "Источник поискового факта");
}

void TSearchContext::SetIntent(const TString& intent) {
    GetAnalyticsInfoBuilder().SetIntentName(intent);
    Builder.SetFeaturesIntent(intent);
}

void TSearchContext::AddDoNothingButton() {
    TFrameNluHint declineNluHint;
    declineNluHint.SetFrameName("alice.proactivity.decline");

    TSemanticFrame doNothingFrame;
    doNothingFrame.SetName("alice.do_nothing");

    NScenarios::TFrameAction actionDecline;
    *actionDecline.MutableNluHint() = std::move(declineNluHint);
    *actionDecline.MutableFrame() = std::move(doNothingFrame);
    AddAction("decline", std::move(actionDecline));
}

void TSearchContext::AddPushPromo() {
    double proba = .0;
    for (const auto& pair: GetRequest().ExpFlags()) {
        if (pair.first.StartsWith(NExperiments::HANDOFF_PROMO_PROBA_PREFIX)) {
            TryFromString(pair.first.substr(NExperiments::HANDOFF_PROMO_PROBA_PREFIX.Size()), proba);
        }
    }
    if (proba > 0. && proba > Rng.RandomDouble()) {
        if (GetRequest().HasExpFlag(NExperiments::HANDOFF_PROMO_LONG)) {
            AddPushPromoButton("handoff_promo_long");
        } else {
            AddPushPromoButton("handoff_promo");
        }
    }
}

void TSearchContext::SetShouldListen(bool shouldListen) {
    ShouldListen = shouldListen;
}

TSearchContext::TSearchContext(const TScenarioRunRequestWrapper& runRequest, TRunResponseBuilder& builder,
                               TContext& ctx, const NScenarios::TRequestMeta& meta, IRng& rng, const NScenarios::TAnalyticsInfo* analyticsInfo)
    : RunRequestWrapper(runRequest)
    , Builder(builder)
    , BodyBuilder(builder.CreateResponseBodyBuilder())
    , Ctx(ctx)
    , Nlg(builder.GetNlgWrapper())
    , LangName(IsoNameByLanguage(Nlg.GetLang()))
    , RequestMeta(meta)
    , ShouldOpenLink(false)
    , ForbidText(false)
    , ShouldListen(true)
    , NlgData(ctx.Logger(), RunRequestWrapper)
    , IsUsingState(false)
    , Rng(rng)
{
    // Note state must be read using framework_migration functions
    ReadScenarioState(runRequest.BaseRequestProto(), State);

    PrepareContextForNlg();
    if (analyticsInfo == nullptr){
        BodyBuilder.CreateAnalyticsInfoBuilder();
    } else {
        BodyBuilder.CreateAnalyticsInfoBuilder(*analyticsInfo);
    }
}

} // namespace NAlice::NHollywood