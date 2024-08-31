#include "walker.h"

#include "combinator_utils.h"
#include "exception.h"
#include "request_frame_to_scenario_matcher.h"
#include "response_visitor.h"
#include "scenario_visitor.h"
#include "talkien.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/combinators.pb.h>
#include <alice/megamind/library/apphost_request/protos/scenario_errors.pb.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/classifiers/features/asr_features.h>
#include <alice/megamind/library/classifiers/features/calcers.h>
#include <alice/megamind/library/classifiers/features/device_state_features.h>
#include <alice/megamind/library/classifiers/features/logging.h>
#include <alice/megamind/library/classifiers/features/query_tokens_features.h>
#include <alice/megamind/library/classifiers/post.h>
#include <alice/megamind/library/classifiers/pre.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/handlers/apphost_megamind/combinators.h>
#include <alice/megamind/library/misspell/misspell.h>
#include <alice/megamind/library/modifiers/context.h>
#include <alice/megamind/library/proactivity/postroll/postroll.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/request/event/server_action_event.h>
#include <alice/megamind/library/request/event/text_input_event.h>
#include <alice/megamind/library/response/builder.h>
#include <alice/megamind/library/response/combinator_response.h>
#include <alice/megamind/library/scenarios/helpers/scenario_api_helper.h>
#include <alice/megamind/library/scenarios/helpers/scenario_ref.h>
#include <alice/megamind/library/scenarios/interface/scenario.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>
#include <alice/megamind/library/scenarios/registry/registry.h>
#include <alice/megamind/library/search/request.h>
#include <alice/megamind/library/sensors/utils.h>
#include <alice/megamind/library/serializers/scenario_proto_deserializer.h>
#include <alice/megamind/library/serializers/speechkit_proto_serializer.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/library/apphost_request/protos/preclassify.pb.h>
#include <alice/megamind/library/apphost_request/protos/scenario.pb.h>
#include <alice/megamind/protos/common/effect_options.pb.h>
#include <alice/megamind/protos/common/origin.pb.h>
#include <alice/megamind/protos/common/subscription_state.pb.h>
#include <alice/megamind/protos/common/tandem_state.pb.h>
#include <alice/megamind/protos/modifiers/modifier_request.pb.h>
#include <alice/megamind/protos/modifiers/modifier_response.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/util.h>
#include <alice/library/analytics/common/names.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/frame/utils.h>
#include <alice/library/metrics/histogram.h>
#include <alice/library/metrics/names.h>
#include <alice/library/metrics/util.h>
#include <alice/library/network/common.h>
#include <alice/library/proto/proto.h>
#include <alice/library/scenarios/data_sources/data_sources.h>
#include <alice/library/typed_frame/typed_frames.h>
#include <alice/library/version/version.h>

#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>

#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/iterator/filtering.h>
#include <library/cpp/iterator/mapped.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/algorithm.h>
#include <util/generic/is_in.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>

#include <algorithm>
#include <functional>
#include <memory>

namespace NAlice {

using EStackEngineEffect = NScenarios::TStackEngineEffect::EffectCase;

namespace NImpl {

void SortWrappers(TScenarioWrapperPtrs& wrappers, const IContext::TExpFlags& expFlags) {
    SortBy(wrappers, [&expFlags](const TScenarioWrapperPtr& wrapper) {
        Y_ASSERT(wrapper);

        ui32 priority = 0;
        OnlyVisit<TConfigBasedAppHostProxyProtocolScenario>(wrapper, [&expFlags, &priority](const TConfigBasedAppHostProxyProtocolScenario& scenario) {
            if (scenario.DependsOnWebSearchResult() || NMegamind::NImpl::DependsOnWebSearchResultByExp(scenario.GetName(), expFlags)) {
                priority = 1;
            }
        });
        OnlyVisit<TConfigBasedAppHostPureProtocolScenario>(wrapper, [&expFlags, &priority](const TConfigBasedAppHostPureProtocolScenario& scenario) {
            if (scenario.DependsOnWebSearchResult() || NMegamind::NImpl::DependsOnWebSearchResultByExp(scenario.GetName(), expFlags)) {
                priority = 1;
            }
        });
        return std::pair<ui32, TStringBuf>{priority, wrapper->GetScenario().GetName()};
    });
}

bool RaiseErrorOnFailedScenarios(const TScenariosErrors& errors, const TVector<TScenarioResponse>& scenarios,
                                 TStringBuf scenarioNames, TRTLogger& logger) {
    bool raiseError = false;
    THashSet<TStringBuf> criticalScenarios{StringSplitter(scenarioNames).Split(';')};

    for (const auto& scenario : scenarios) {
        if (scenario.GetHttpCode() == HTTP_UNASSIGNED_512 && criticalScenarios.contains(scenario.GetScenarioName())) {
            LOG_ERR(logger) << "Scenario error: " << scenario.GetScenarioName() << " returned 512";
            raiseError = true;
        }
    }

    auto findRaisingErrorScenarios = [&logger, &raiseError, &criticalScenarios](const TString& scenarioName,
                                                                                const TString& /* stage */,
                                                                                const TError& /* error */) {
        if (criticalScenarios.contains(scenarioName) || criticalScenarios.contains(NImpl::ALL_SCENARIOS)) {
            LOG_ERR(logger) << "Scenario error: " << scenarioName << " failed to respond";
            raiseError = true;
        }
    };

    errors.ForEachError(findRaisingErrorScenarios);

    return raiseError;
}

bool ShouldUseFormulasForRanking(const TScenario& scenario, const IContext& ctx) {
    const auto& classificationConfig = ctx.ClassificationConfig().GetScenarioClassificationConfigs();
    const auto* config = MapFindPtr(classificationConfig, scenario.GetName());
    const bool ignoreUsingFormulaForGc = scenario.GetName() == PROTOCOL_GENERAL_CONVERSATION_SCENARIO;
    return config && config->GetUseFormulasForRanking() && !ignoreUsingFormulaForGc;
}

void AddSearchRelatedScenarioStats(const IContext& ctx, TScenarioWrapperPtr wrapper,
                                   TCommonScenarioWalker::EHeavyScenarioEvent eventKind) {
    Y_ASSERT(wrapper);
    if (!wrapper->IsSuccess()) {
        return;
    }

    const auto& scenario = wrapper->GetScenario();
    const bool dependsOnWebSearchResult = scenario.DependsOnWebSearchResult() || NMegamind::NImpl::DependsOnWebSearchResultByExp(scenario.GetName(), ctx.ExpFlags());
    if (dependsOnWebSearchResult || ShouldUseFormulasForRanking(scenario, ctx)) {
        const auto& client = ctx.SpeechKitRequest()->GetApplication().GetAppId();
        NMonitoring::TLabels labels = NSignal::SCENARIOS_WITH_SEARCH;
        labels.Add(NSignal::CLIENT_TYPE, NSignal::GetClientType(client));
        labels.Add(NSignal::EVENT, ToString(eventKind));
        labels.Add(NSignal::SCENARIO_NAME, scenario.GetName());
        ctx.Sensors().IncRate(labels);
    }
}


bool IsActiveConditionalDatasource(const TConfig::TScenarios::TConfig::TDataSourceConditions& dsConditions,
                                   const TVector<TSemanticFrame>& frames, const ELanguage language,
                                   std::function<bool(const TStringBuf)> hasExpFlag)
{
    using TDataSourceCondition = TConfig::TScenarios::TConfig::TDataSourceConditions::TDataSourceCondition;
    for (const auto& condition : dsConditions.GetConditions()) {
        switch (condition.GetConditionCase()) {
            case TDataSourceCondition::ConditionCase::kOnSemanticFrameCondition: {
                bool result = AnyOf(frames, [&condition](const auto& frame) {
                    return frame.GetName() == condition.GetOnSemanticFrameCondition().GetSemanticFrameName();
                });
                if (result) {
                    return true;
                }
                break;
            }
            case TDataSourceCondition::ConditionCase::kOnUserLanguage: {
                if (static_cast<ELanguage>(condition.GetOnUserLanguage().GetLanguage()) == language) {
                    return true;
                }
                break;
            }
            case TDataSourceCondition::ConditionCase::kOnExperimentFlag: {
                if (hasExpFlag(condition.GetOnExperimentFlag().GetFlagName())) {
                    return true;
                }
                break;
            }
            case TDataSourceCondition::ConditionCase::CONDITION_NOT_SET:
                return false;
        }
    }
    return false;
}

bool ShouldMakeSearchRequest(const TScenarioToRequestFrames& scenarioToRequestFrames,
                             const IContext& ctx) {
    bool foundScenarioNeedFormulas = false;
    for (const auto& [ref, frames] : scenarioToRequestFrames) {
        const auto& scenario = ref->GetScenario();
        const bool dependsOnWebSearchResult = scenario.DependsOnWebSearchResult() || NMegamind::NImpl::DependsOnWebSearchResultByExp(scenario.GetName(), ctx.ExpFlags());
        if (dependsOnWebSearchResult) {
            return true;
        }
        if (ShouldUseFormulasForRanking(scenario, ctx)) {
            if (foundScenarioNeedFormulas) {
                return true;
            }
            foundScenarioNeedFormulas = true;
        }

        // Check conditional datasources
        for (const auto& conditionalDatasourceConfig : ctx.ScenarioConfig(scenario.GetName()).GetConditionalDataSources()) {
            bool isWebSearchDatasource = AnyOf(WEB_SOURCES, [&conditionalDatasourceConfig](const auto& webSource) {
                return conditionalDatasourceConfig.GetDataSourceType() == webSource;
            });
            if (!isWebSearchDatasource) {
                continue;
            }
            if (IsActiveConditionalDatasource(conditionalDatasourceConfig, frames, ctx.Language(),
                                              [&ctx](const TStringBuf expName) { return ctx.HasExpFlag(expName); })) {
                return true;
            }
        }
    }
    return false;
}

bool HasScenario(const TScenarioToRequestFrames& scenarioToRequestFrames,
                 const TStringBuf& scenarioName) {
    return AnyOf(scenarioToRequestFrames, [scenarioName](const auto& pair){
        return pair.first->GetScenario().GetName() == scenarioName;});
}

void CopyScenarioErrorsToWinnerResponse(const TWalkerResponse& walkerResponse, TScenarioResponse& topResponse,
                                        const IContext& ctx) {
    if (auto* builder = topResponse.BuilderIfExists()) {
        walkerResponse.GetScenarioErrors().ForEachError(
            [&builder, &ctx](const TString& scenarioName, const TString& stage, const TError& error) {
                NMegamind::IncErrorOnTestIds(ctx.Sensors(), ctx.SpeechKitRequest()->GetRequest().GetTestIDs(),
                                             NMegamind::ETestIdErrorType::SCENARIO_ERROR, /* labels= */ {});
                builder->AddError(ToString(error.Type),
                                  TScenariosErrors::ErrorToString(scenarioName, stage, error));
            });
    }
}

THashSet<TString> GetScenariosFromFlag(const TMaybe<TString>& scenarios) {
    if (!scenarios.Defined()) {
        return {ALL_SCENARIOS};
    }
    return StringSplitter(*scenarios).Split(';');
}

TMaybe<NAlice::NData::TContactsList> GetContacts(const TSpeechKitRequest& skr) {
    if (skr->GetContacts().GetStatus() == "ok") {
        return skr->GetContacts().GetData();
    }
    return Nothing();
}

TScenarioWrapperPtrs MakeScenarioWrappers(const TScenarioToRequestFrames& scenarioToRequestFrames,
                                          const IContext& ctx,
                                          const NMegamind::IGuidGenerator& guidGenerator,
                                          EDeferredApplyMode deferApplyMode,
                                          NMegamind::TItemProxyAdapter& itemProxyAdapter)
{
    TScenarioWrapperPtrs wrappers;
    for (const auto& [ref, requestFrames] : scenarioToRequestFrames) {
        TScenarioWrapperPtr wrapper;
        ref->Accept(TScenarioWrapperFactory(
            ctx,
            requestFrames,
            guidGenerator,
            deferApplyMode,
            wrapper,
            itemProxyAdapter
        ));
        Y_ASSERT(wrapper);
        wrappers.push_back(wrapper);
    }

    NImpl::SortWrappers(wrappers, ctx.ExpFlags());

    return wrappers;
}

bool HasCriticalScenarioVersionMismatch(const TScenariosErrors& errors, TRTLogger& logger) {
    bool hasCriticalMismatch = false;
    errors.ForEachError(
        [&logger, &hasCriticalMismatch](const TString& scenarioName, const TString& /* stage */, const TError& error) {
            if (error.Type == TError::EType::VersionMismatch) {
                LOG_ERR(logger) << "Critical scenario version mismatch: " << scenarioName;
                hasCriticalMismatch = true;
            }
        });
    return hasCriticalMismatch;
}

} // namespace NImpl

namespace {

using TActionEffect = TCommonScenarioWalker::TActionEffect;

TStatus FinalizeWrapperAndPostFillAnalytics(TScenarioWrapperPtr wrapper,
                                            const TPostAnalyticsFiller& postAnalyticsFiller, const TRequest& request,
                                            const IContext& ctx, TScenarioResponse& response) {
    if (!wrapper) {
        return TError{TError::EType::Logic} << "Wrapper is not set in FinalizeWrapperAndPostFillAnalytics";
    }
    const auto error = wrapper->Finalize(request, ctx, response);
    postAnalyticsFiller(wrapper);
    if (error.Defined()) {
        return TError{TError::EType::Logic} << "Scenario " << response.GetScenarioName() << " wrapper finalize error: " << *error;
    }
    return Success();
}

NMegamind::TProactivityStorage GetProactivityStorage(const IContext& ctx) {
    const auto& proactivityConfigStorage = ctx.MementoData().GetUserConfigs().GetProactivityConfig().GetStorage();
    return GetProactivityStorage(ctx.SpeechKitRequest(), ctx.Session(), proactivityConfigStorage, &ctx.Logger());
}

void AskSkillProactivity(IRunWalkerRequestCtx& walkerCtx,
                         const TRequest& requestModel,
                         const TScenarioToRequestFrames& scenarioToFrames)
{
    using namespace NAlice::NMegamind;

    auto& ctx = walkerCtx.Ctx();
    const TString notAskMessage = "Will not ask SkillProactivity: ";
    if (!IsProactivityAllowedInApp<IContext>(ctx)) {
        LOG_INFO(ctx.Logger()) << notAskMessage << "disabled in app";
        return;
    }
    if (ctx.SpeechKitRequest().Event().GetType() != EEventType::voice_input && !ctx.HasExpFlag(EXP_PROACTIVITY_ENABLE_ON_ANY_EVENT)) {
        LOG_INFO(ctx.Logger()) << notAskMessage << "disabled for non-voice requests";
        return;
    }

    const bool hasFrequentSources = ProactivityHasFrequentSources(requestModel, scenarioToFrames);
    const auto proactivityStorage = GetProactivityStorage(ctx);

    ui64 minutesSinceLast = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_PROACTIVITY_TIME_DELTA_THRESHOLD_PREFIX, PROACTIVITY_TIME_DELTA_THRESHOLD);
    if (minutesSinceLast != 0 && (ctx.ClientInfo().Epoch < proactivityStorage.GetLastShowTime() ||
        (ctx.ClientInfo().Epoch - proactivityStorage.GetLastShowTime() < minutesSinceLast * 60 && !hasFrequentSources))
    ) {
        LOG_INFO(ctx.Logger()) << notAskMessage << "disabled by timedelta_threshold=" << minutesSinceLast
                               << ", CurrentEpoch=" << ctx.ClientInfo().Epoch
                               << ", LastShowTime=" << proactivityStorage.GetLastShowTime();
        return;
    }

    ui64 requestsSinceLast = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_PROACTIVITY_REQUEST_DELTA_THRESHOLD_PREFIX, PROACTIVITY_REQUEST_DELTA_THRESHOLD);
    if (requestsSinceLast != 0 && (proactivityStorage.GetRequestCount() < proactivityStorage.GetLastShowRequestCount() ||
        (proactivityStorage.GetRequestCount() - proactivityStorage.GetLastShowRequestCount() < requestsSinceLast && !hasFrequentSources))
    ) {
        LOG_INFO(ctx.Logger()) << notAskMessage << "disabled by request_delta_threshold=" << requestsSinceLast
                               << ", CurrentRequestCount=" << proactivityStorage.GetRequestCount()
                               << ", LastShowRequestCount=" << proactivityStorage.GetLastShowRequestCount();
        return;
    }

    walkerCtx.MakeProactivityRequest(requestModel, scenarioToFrames, proactivityStorage);
}

TString GetWebSearchQuery(const IContext& ctx, const IEvent& event) {
    if (const auto& query = ctx.Responses().WebSearchQueryResponse().GetQuery(); !query.empty()) {
        return query;
    }
    if (const auto query = ctx.Responses().WizardResponse().GetSearchQuery(ctx.ExpFlags()); query.Defined() && event.IsUserGenerated()) {
        LOG_DEBUG(ctx.Logger()) << "Using wizard result for search query: " << *query;
        return *query;
    }
    return event.GetUtterance();
}

void AskWebSearch(const TRequest& request, IRunWalkerRequestCtx& walkerCtx,
                  const TScenarioToRequestFrames& scenarioToRequestFrames)
{
    auto& requestCtx = walkerCtx.RequestCtx();
    auto& sensors = walkerCtx.Ctx().Sensors();

    if (!NImpl::ShouldMakeSearchRequest(scenarioToRequestFrames, walkerCtx.Ctx())) {
        return;
    }
    const auto& event = request.GetEvent();
    if (!event.HasUtterance() && !event.GetUtterance().empty()) {
        return;
    }

    auto& ctx = walkerCtx.Ctx();

    TWebSearchRequestBuilder builder{GetWebSearchQuery(ctx, event)};

    if (const auto& userTicket = ctx.Responses().BlackBoxResponse().GetUserTicket(); !userTicket.Empty()) {
        builder.SetUserTicket(userTicket);
    }

    if (const auto& uid = ctx.Responses().BlackBoxResponse().GetUserInfo().GetUid(); !uid.Empty()) {
        builder.SetUid(uid);
    }

    builder.SetUserRegion(request.GetUserLocation().UserRegion());
    builder.SetContentSettings(ToProtoType(request.GetContentRestrictionLevel()));
    builder.SetSensors(sensors);

    const auto& requestFrames = request.GetSemanticFrames();
    const bool hasImageSearchGranet = AnyOf(requestFrames, [](const TSemanticFrame& frame){
        return frame.GetName() == MM_SEARCH_IMAGES_FRAME;
    });
    if (hasImageSearchGranet) {
        builder.SetHasImageSearchGranet();
    }

    walkerCtx.MakeSearchRequest(builder, event);

    // TODO (petrk) A temporary signal to understand where the slowdown is!
    requestCtx.StageTimers().RegisterAndSignal(requestCtx, TStringBuilder{} << NMegamind::TS_STAGE_WEBSEARCH_REQUESTED << '.' << walkerCtx.RunStage(),
                                               NMegamind::TS_STAGE_START_REQUEST, walkerCtx.Ctx().Sensors());
}

TScenarioWrapperPtr FindWrapperByScenarioName(const TString& scenarioName, const TScenarioWrapperPtrs& wrappers) {
    for (const auto& wrapper : wrappers) {
        if (wrapper->GetScenario().GetName() == scenarioName) {
            return wrapper;
        }
    }
    return {};
}

TScenarioWrapperPtr FindWrapper(const TScenarioResponse& response, const TScenarioWrapperPtrs& wrappers) {
    return FindWrapperByScenarioName(response.GetScenarioName(), wrappers);
}

void FillScenariosFactorStorage(const TVector<TScenarioResponse>& scenarioResponses, TFactorStorage& storage) {
    for (const auto& responses : scenarioResponses) {
        const auto& features = responses.GetFeatures().GetScenarioFeatures();
        FillScenarioFactors(features, storage);
    }
}

// TODO(jan-fazli) create ModifierRegistry
TVector<NMegamind::TModifierPtr> MakeModifiers() {
    TVector<NMegamind::TModifierPtr> modifiers;
    modifiers.emplace_back(NMegamind::CreatePostrollModifier());
    return modifiers;
}

TEvent GetCallbackEventFromStruct(const google::protobuf::Struct& directive) {
    return StructToMessage<TEvent>(directive);
}

TMaybe<TEvent> TryGetSpeechKitCallbackEvent(const NScenarios::TCallbackDirective& callback,
                                            const TString& scenarioName, const TString& requestId,
                                            const TSpeechKitRequest& speechKitRequest,
                                            const TMaybe<TIoTUserInfo>& iotUserInfo,
                                            TRTLogger& logger)
{
    const NMegamind::TSerializerMeta meta{scenarioName, requestId,
                                          speechKitRequest.ClientInfo(),
                                          iotUserInfo,
                                          speechKitRequest->GetRequest().GetSmartHomeInfo()};
    const NMegamind::TScenarioProtoDeserializer deserializer{meta, logger};
    if (const auto directiveModel = deserializer.Deserialize(callback); directiveModel) {
        const NMegamind::TSpeechKitProtoSerializer serializer{meta};
        const auto speechKitCallback = serializer.Serialize(*directiveModel);
        TEvent event{};
        if (speechKitCallback.HasPayload()) {
            *event.MutablePayload() = speechKitCallback.GetPayload();
        }
        event.SetType(EEventType::server_action);
        event.SetName(speechKitCallback.GetName());
        event.SetIgnoreAnswer(speechKitCallback.GetIgnoreAnswer());
        return event;
    }
    return Nothing();
}

TMaybe<TEvent> TryGetSpeechKitCallbackEvent(const NScenarios::TCallbackDirective& callback, const ISession* session,
                                            const TString& prevReqId, const TSpeechKitRequest& speechKitRequest,
                                            const TMaybe<TIoTUserInfo>& iotUserInfo, TRTLogger& logger)
{
    if (!session) {
        return Nothing();
    }

    return TryGetSpeechKitCallbackEvent(callback, session->GetPreviousScenarioName(),
                                        prevReqId, speechKitRequest, iotUserInfo, logger);
}

TActionEffect GetActionEffect(const IContext& ctx, const TMaybe<TIoTUserInfo>& iotUserInfo,
                              NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                              TWalkerResponse& response) {
    if (const auto conditionalEffect = TryGetConditionalEffect(ctx, analyticsInfoBuilder);
        conditionalEffect.Defined())
    {
        return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
                .UpdatedEvent = {},
                .EffectFrame = Nothing(),
                .ActionFrames = {*conditionalEffect}};
    }
    if (const auto actionResponse = TryGetActionResponse(ctx, iotUserInfo, analyticsInfoBuilder)) {
        if (const auto* directiveListResponse = std::get_if<TDirectiveListResponse>(actionResponse.Get())) {
            response = TWalkerResponse{*directiveListResponse};
            return {.Status = TActionEffect::EStatus::WalkerResponseIsComplete,
                    .UpdatedEvent = {},
                    .EffectFrame = Nothing()};
        } else if (const auto* callback = std::get_if<NScenarios::TCallbackDirective>(actionResponse.Get())) {
            const TString& prevReqId = ctx.SpeechKitRequest().Proto().GetHeader().GetPrevReqId();
            const auto skCallbackEvent = TryGetSpeechKitCallbackEvent(
                *callback, ctx.Session(), prevReqId, ctx.SpeechKitRequest(), iotUserInfo, ctx.Logger());
            if (!skCallbackEvent.Defined()) {
                LOG_ERROR(ctx.Logger()) << "Bad callback directive on action: " << SerializeProtoText(*callback);
                return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
                        .UpdatedEvent = {},
                        .EffectFrame = Nothing()};
            }

            if (const auto* parentProductScenario = MapFindPtr(skCallbackEvent->GetPayload().fields(),
                                                               MM_CALLBACK_PARENT_PRODUCT_SCENARIO_NAME)) {
                analyticsInfoBuilder.SetParentProductScenarioName(parentProductScenario->string_value());
            }

            return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
                    .UpdatedEvent = IEvent::CreateEvent(*skCallbackEvent),
                    .EffectFrame = Nothing()};
        } else if (const auto* parsedUtterance = std::get_if<NScenarios::TParsedUtterance>(actionResponse.Get())) {
            if (parsedUtterance->HasFrame()) {
                LOG_WARNING(ctx.Logger()) << "Got `ParsedUtterance` effect with deprecated field `Frame` set.";
                return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
                        .UpdatedEvent = std::make_unique<TTextInputEvent>(parsedUtterance->GetUtterance(),
                                                                          /* isUserGenerated= */ false),
                        .EffectFrame = parsedUtterance->GetFrame()};
            }
            const TTypedSemanticFrameRequest typedSemanticFrameRequest{*parsedUtterance,
                                                                                  /* validateAnalytics= */ false};
            analyticsInfoBuilder.SetParentProductScenarioName(typedSemanticFrameRequest.ProductScenario);
            return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
                    .UpdatedEvent = std::make_unique<TTextInputEvent>(typedSemanticFrameRequest.Utterance,
                                                                      /* isUserGenerated= */ false),
                    .EffectFrame = typedSemanticFrameRequest.SemanticFrame};
        } else if (const auto* semanticFrame = std::get_if<TSemanticFrame>(actionResponse.Get())) {
            return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
                    .UpdatedEvent = {},
                    .EffectFrame = *semanticFrame};
        } else if (const auto* actionFrames = std::get_if<TVector<TSemanticFrameRequestData>>(actionResponse.Get())) {
            return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
                    .ActionFrames = *actionFrames};
        }
    }
    return {.Status = TActionEffect::EStatus::WalkerResponseIsNotComplete,
            .UpdatedEvent = {},
            .EffectFrame = Nothing()};
}

void WritePerScenarioGetNextMetrics(const TString& scenarioName, const size_t stackSize, NMetrics::ISensors& sensors) {
    sensors.IncRate(NSignal::LabelsForScenarioInStack(scenarioName, NSignal::STACK_REQUESTS_PER_SECOND));
    sensors.AddHistogram(NSignal::LabelsForScenarioInStack(scenarioName, NSignal::STACK_SIZE),
                         stackSize, NMetrics::SMALL_SIZE_INTERVALS);
}

template <typename TOnCallbackFn>
TStatus OnGetNextRequest(NMegamind::TStackEngine& stackEngine, TRTLogger& logger, IFrameRequestProcessor& frameRequestProcessor,
                         TOnCallbackFn&& onCallback, TEffectOptions& effectOptions,
                         const TSpeechKitRequest& speechKitRequest, NMetrics::ISensors& sensors,
                         const TMaybe<TIoTUserInfo>& iotUserInfo) {
    bool processed = false;
    while (!stackEngine.IsEmpty()) {
        const auto item = stackEngine.Pop();
        const auto& effect = item.GetEffect();
        const auto effectCase = effect.GetEffectCase();
        switch (effect.GetEffectCase()) {
            case EStackEngineEffect::kCallback: {
                const auto skCallback = TryGetSpeechKitCallbackEvent(effect.GetCallback(), item.GetScenarioName(),
                                                                     /* requestId= */ Default<TString>(),
                                                                     speechKitRequest, iotUserInfo, logger);
                if (skCallback.Defined()) {
                    onCallback(*skCallback);
                    processed = true;
                } else {
                    LOG_WARNING(logger) << "Pop invalid callback from stack engine: " << effect.GetCallback();
                }
                break;
            }
            case EStackEngineEffect::kParsedUtterance: {
                frameRequestProcessor.Process(TTypedSemanticFrameRequest{effect.GetParsedUtterance(),
                                                                                    /* validateAnalytics= */ false});
                processed = true;
                break;
            }
            default: {
                sensors.IncRate(NSignal::LabelsForStackErrorsPerScenario(item.GetScenarioName(), NSignal::STACK_ENGINE_INVALID_EFFECT));
                LOG_WARNING(logger) << "Pop invalid effect from stack engine: " << static_cast<int>(effectCase);
                break;
            }
        }
        if (processed) {
            WritePerScenarioGetNextMetrics(item.GetScenarioName(), stackEngine.GetCore().ItemsSize() + 1, sensors);
            effectOptions = effect.GetOptions();
            break;
        }
    }
    if (processed) {
        return Success();
    }
    return TError{TError::EType::Logic} << "Stack engine is empty";
}

TRequest::TParameters GetParameters(const TEffectOptions& effectOptions) {
    TMaybe<bool> forcedShouldListen;
    if (effectOptions.HasForcedShouldListen()) {
        forcedShouldListen = effectOptions.GetForcedShouldListen().value();
    }
    TMaybe<TDirectiveChannel::EDirectiveChannel> channel;
    if (effectOptions.HasChannel()) {
        channel = effectOptions.GetChannel();
    }
    TMaybe<TString> forcedEmotion;
    if (effectOptions.HasForcedEmotion()) {
        forcedEmotion = effectOptions.GetForcedEmotion();
    }
    return {forcedShouldListen, channel, forcedEmotion};
}

TVector<TSemanticFrame> EnrichSemanticFrames(TVector<TSemanticFrame> semanticFrames) {
    for (auto& semanticFrame : semanticFrames) {
        if (!semanticFrame.HasTypedSemanticFrame()) {
            if (auto typedFrame = TryMakeTypedSemanticFrameFromSemanticFrame(semanticFrame);
                typedFrame.Defined()) {
                *semanticFrame.MutableTypedSemanticFrame() = std::move(typedFrame.GetRef());
            }
        }
    }
    return semanticFrames;
}

TStatus RestoreFactorStorage(IRunWalkerRequestCtx& walkerCtx, TErrorOr<NMegamindAppHost::TFactorStorageBinaryProto>&& result) {
    try {
        NMegamindAppHost::TFactorStorageBinaryProto proto;
        result.MoveTo(proto);
        TStringInput strInputStream(proto.GetFactorStorage());
        NFSSaveLoad::Deserialize(&strInputStream, NFactorSlices::TGlobalSlicesMetaInfo::Instance(), &walkerCtx.FactorStorage());
    } catch (...) {
        auto err = TError{TError::EType::Parse} << "Unable to deserialize factor storage: " << CurrentExceptionMessage();
        LOG_WARN(walkerCtx.Ctx().Logger()) << err;
        return std::move(err);
    }

    return Success();
}

void RestorePreClassifyData(IScenarioWalker::TPreClassifyState& preClassifyState, TErrorOr<NMegamindAppHost::TPreClassifyProto>&& result) {
    NMegamindAppHost::TPreClassifyProto proto;
    result.MoveTo(proto);

    preClassifyState.IsTrashPartial = proto.GetIsTrashPartial();
}

void RestoreQualityStorage(IScenarioWalker::TPreClassifyState& preClassifyState, TErrorOr<TQualityStorage>&& result) {
    result.MoveTo(preClassifyState.QualityStorage);
}

TStatus RestoreAnalytics(TWalkerResponse& walkerResponse, NMegamind::IPostClassifyState& postClassifyState) {
    auto analytics = postClassifyState.GetAnalytics();
    if (analytics.Error()) {
        return std::move(*analytics.Error());
    }
    walkerResponse.AnalyticsInfoBuilder.CopyFromProto(std::move(analytics.Value()));
    return Success();
}

TStatus RestoreQualityStorage(TWalkerResponse& walkerResponse, NMegamind::IPostClassifyState& postClassifyState) {
    auto qualityStorage = postClassifyState.GetQualityStorage();
    if (qualityStorage.Error()) {
        return std::move(*qualityStorage.Error());
    }
    qualityStorage.MoveTo(walkerResponse.QualityStorage);
    return Success();
}

void RestoreScenarioErrors(TWalkerResponse& walkerResponse, NMegamind::IPostClassifyState& postClassifyState) {
    auto errors = postClassifyState.GetScenarioErrors();
    if (!errors.Defined()) {
        return;
    }
    for (const auto& scenarioError : errors->GetScenarioErrors()) {
        walkerResponse.AddScenarioError(scenarioError.GetScenario(), scenarioError.GetStage(),
                                        NMegamind::ErrorFromProto(scenarioError.GetError()));
    }
}

void RestoreScenarios(const TScenarioRegistry& scenarioRegistry,
                      NMegamind::TItemProxyAdapter& itemAdapter,
                      TScenarioToRequestFrames& scenariosToRequestFrames)
{
    THashMap<TString, NMegamindAppHost::TScenarioProto> scenariosProtos;
    auto onItem = [&scenariosProtos](NMegamindAppHost::TScenarioProto& proto) {
        scenariosProtos[proto.GetName()] = proto;
    };
    if (!itemAdapter.ForEachCached<NMegamindAppHost::TScenarioProto>(NMegamind::AH_ITEM_SCENARIO, onItem)) {
        if (!scenariosProtos.empty()) {
            scenariosToRequestFrames = MatchScenarios(scenariosProtos, scenarioRegistry.GetScenarioRefs());
        }
    }
}

TProactivityAnswer WaitProactivity(NMegamind::TItemProxyAdapter& itemProxyAdapter,
                                   const IContext& ctx,
                                   const TScenarioResponse& scenarioResponse) {
    auto& logger = ctx.Logger();
    TProactivityAnswer proactivity;
    if (itemProxyAdapter.CheckFlagInInputContext(NMegamind::AH_FLAG_EXPECT_PROACTIVITY_RESPONSE)) {
        TStatus error;
        const NDJ::NAS::TProactivityResponse& response = ctx.Responses().ProactivityResponse(&error);
        if (error) {
            LOG_ERR(logger) << "SkillProactivity request error: " << *error;
        } else {
            const auto proactivitySource = NMegamind::GetProactivitySource(ctx.Session(), scenarioResponse);
            proactivity = NMegamind::GetProactivityRecommendations(response, proactivitySource, logger);
        }
    }
    return proactivity;
}

void CheckFinalResponses(const IContext& ctx, TScenarioResponse& scenarioResponse, TWalkerResponse& response) {
    auto& logger = ctx.Logger();
    if (const auto& scenarios = ctx.ExpFlag(EXP_RAISE_ERROR_ON_FAILED_SCENARIOS); scenarios.Defined()) {
        if (NImpl::RaiseErrorOnFailedScenarios(response.Errors, response.Scenarios, *scenarios, logger)) {
            scenarioResponse.SetHttpCode(HTTP_UNASSIGNED_512, "Raised error on failed scenarios");
        }
    }

    if (NImpl::HasCriticalScenarioVersionMismatch(response.Errors, logger)) {
        scenarioResponse.SetHttpCode(HTTP_UNASSIGNED_512, "Has critical scenario version mismatch");
    }
}

THashSet<TString> GetScenariosWithTunnellerResponses(const IContext& ctx) {
    return NImpl::GetScenariosFromFlag(ctx.ExpFlag(EXP_MOVE_TUNNELLER_RESPONSES_FROM_SCENARIOS));
}

void FillAdditionalFrameSubscription(const IContext& ctx, THashMap<TString, TVector<TString>>& scenariosToFrames) {
    for (const auto& [flag, _] : ctx.ExpFlags()) {
        TStringBuf values;
        if (flag.StartsWith(EXP_PREFIX_SUBSCRIBE_TO_FRAME)) {
            TVector<TStringBuf> parts;
            TStringBuf{flag}.AfterPrefix(EXP_PREFIX_SUBSCRIBE_TO_FRAME, values);
            Split(values, ":", parts);
            if (parts.size() != 2) {
                LOG_WARN(ctx.Logger()) << "Malformed subscription experiment: " << flag;
                continue;
            }
            TVector<TString> newFrames;
            Split(TString{parts[1]}, ",", newFrames);
            const TString scenarioName{parts[0]};
            auto& frames = scenariosToFrames[scenarioName];
            frames.insert(frames.end(), newFrames.begin(), newFrames.end());
        }
    }
}

TString ConstructConditionalDatasourceFlag(const TString& scenarioName, EDataSourceType datasource) {
    return "need_conditional_datasource_" + scenarioName + "_" + EDataSourceType_Name(datasource);
}

} // namespace

namespace NImpl {

TErrorOr<TScenarioWrapperPtrs> RestoreInitializedWrappers(NMegamind::TItemProxyAdapter& itemAdapter,
                                                          const TScenarioWrapperPtrs& wrappers) {
    auto launchedScenarios =
        itemAdapter.GetFromContext<NMegamindAppHost::TLaunchedScenarios>(NMegamind::AH_ITEM_LAUNCHED_SCENARIOS);

    if (launchedScenarios.Error()) {
        return std::move(*launchedScenarios.Error());
    }
    TVector<TString> scenarios(Reserve(launchedScenarios.Value().GetScenarios().size()));
    for (auto& scenario : *launchedScenarios.Value().MutableScenarios()) {
        scenarios.push_back(std::move(*scenario.MutableName()));
    }
    Sort(scenarios);
    TScenarioWrapperPtrs initializedWrappers;
    for (auto& wrapper : wrappers) {
        if (BinarySearch(scenarios.begin(), scenarios.end(), wrapper->GetScenario().GetName())) {
            initializedWrappers.push_back(wrapper);
        }
    }
    return std::move(initializedWrappers);
}

void PushFlagsForConditionalDatasources(NMegamind::TItemProxyAdapter& itemAdapter, const IContext& ctx,
                                        TScenarioWrapperPtrs scenarioWrappers) {
    for (const auto& scenarioWrapper : scenarioWrappers) {
        const auto& scenarioName = scenarioWrapper->GetScenario().GetName();
        for (const auto& conditionalDatasourceConfig : ctx.ScenarioConfig(scenarioName).GetConditionalDataSources()) {
            if (IsActiveConditionalDatasource(conditionalDatasourceConfig, scenarioWrapper->GetSemanticFrames(),
                                              ctx.Language(),
                                              [&ctx](const TStringBuf expName) { return ctx.HasExpFlag(expName); })) {
                itemAdapter.PutIntoContext(google::protobuf::StringValue{},
                    ConstructConditionalDatasourceFlag(scenarioName, conditionalDatasourceConfig.GetDataSourceType()));
            }
        }
    }
}

bool ProcessActionEffect(IScenarioWalker::TActionEffect& actionEffect, IScenarioWalker::TRunState& runState,
                         IFrameRequestProcessor& frameRequestProcessor,
                         bool& utteranceWasUpdated, std::unique_ptr<const IEvent>& event,
                         TVector<TSemanticFrame>& recognizedActionEffectFrames, TVector<TSemanticFrame>& forcedSemanticFrames,
                         TRTLogger& logger, bool doNotForceActionEffectFrameWhenNoUtteranceUpdate) {
    if (actionEffect.Status == TActionEffect::EStatus::WalkerResponseIsComplete) {
        runState.Response.AnalyticsInfoBuilder = std::move(runState.AnalyticsInfoBuilder);
        return false;
    }

    if (actionEffect.UpdatedEvent) {
        event = std::move(actionEffect.UpdatedEvent);
        if (event->HasUtterance()) {
            utteranceWasUpdated = true;
        }
    }

    Y_ENSURE(!actionEffect.EffectFrame.Defined() || actionEffect.ActionFrames.empty());

    if (actionEffect.EffectFrame.Defined()) {
        recognizedActionEffectFrames = {*actionEffect.EffectFrame};
        if (!doNotForceActionEffectFrameWhenNoUtteranceUpdate || utteranceWasUpdated) {
            forcedSemanticFrames.push_back(*actionEffect.EffectFrame);
        }
    }
    const auto& actionFrames = actionEffect.ActionFrames;
    switch (actionFrames.size()) {
        case 0:
            break;
        case 1:
            LOG_INFO(logger) << "Got SemanticFrame request from ActiveActions";
            frameRequestProcessor.Process(TTypedSemanticFrameRequest{actionFrames.front()});
            break;
        default:
            LOG_INFO(logger) << "Got Semantic Frame requests (#" << actionFrames.size() << ") from ActiveActions";
            for (const auto& actionFrame : actionFrames) {
                const TTypedSemanticFrameRequest frameRequest{actionFrame};
                recognizedActionEffectFrames.push_back(frameRequest.SemanticFrame);
                if (!doNotForceActionEffectFrameWhenNoUtteranceUpdate || utteranceWasUpdated) {
                    forcedSemanticFrames.push_back(frameRequest.SemanticFrame);
                }
                if (!frameRequest.ProductScenario.empty()) {
                    runState.AnalyticsInfoBuilder.SetParentProductScenarioName(frameRequest.ProductScenario);
                }
            }
            break;
    }
    return true;
}

TErrorOr<TScenarioWrapperPtr> RestoreWinner(TWalkerResponse& walkerResponse, const TString& winnerName,
                                            TScenarioWrapperPtrs wrappers, IContext& ctx, const TRequest& request,
                                            TMaybe<NScenarios::TScenarioContinueResponse> continueResponse) {
    auto wrapper = FindWrapperByScenarioName(winnerName, wrappers);
    walkerResponse.AddScenarioResponse(TScenarioResponse{
        wrapper->GetScenario().GetName(), wrapper->GetSemanticFrames(), wrapper->GetScenario().AcceptsAnyUtterance()});

    auto& scenarioResponse = walkerResponse.Scenarios.front();
    wrapper->Ask(request, ctx, scenarioResponse);
    if (continueResponse.Defined()) {
        scenarioResponse.SetContinueResponse(*continueResponse);
        if (auto status = wrapper->FinishContinue(request, ctx, scenarioResponse); status.Defined()) {
            return std::move(*status);
        }
    }
    return wrapper;
}

void TFrameRequestProcessor::Process(const TTypedSemanticFrameRequest& frameRequest) {
    Origin = frameRequest.Origin;
    RecognizedActionEffectFrames.get() = {frameRequest.SemanticFrame};
    ForcedSemanticFrames.get().push_back(frameRequest.SemanticFrame);
    ProcessParentProductScenarioName(frameRequest.ProductScenario);
    DisableVoiceSession = frameRequest.DisableVoiceSession;
    DisableShouldListen = frameRequest.DisableShouldListen;
    Event.get() = std::make_unique<TTextInputEvent>(frameRequest.Utterance, /* isUserGenerated= */ false);
    LOG_INFO(Logger.get()) << "Event's been changed to: " << Event.get()->SpeechKitEvent();
    LOG_INFO(Logger.get()) << "Forced semantic frames from request: " << GetFrameNameListString(ForcedSemanticFrames);
}


} // namespace NImpl

// TCommonScenarioWalker ------------------------------------------------------
TCommonScenarioWalker::TCommonScenarioWalker(IGlobalCtx& globalCtx) {
    for (const auto& [_, config] : globalCtx.ScenarioConfigRegistry().GetScenarioConfigs()) {
        if (config.GetHandlers().GetRequestType() == NAlice::ERequestType::AppHostProxy) {
            ScenarioRegistry.RegisterConfigBasedAppHostProxyProtocolScenario(
                    MakeHolder<TConfigBasedAppHostProxyProtocolScenario>(config));
        } else if (config.GetHandlers().GetRequestType() == NAlice::ERequestType::AppHostPure) {
            ScenarioRegistry.RegisterConfigBasedAppHostPureProtocolScenario(
                    MakeHolder<TConfigBasedAppHostPureProtocolScenario>(config));
        } else {
            ythrow yexception{} << "Unknown RequestType for scenario " << config.GetName() << ": " << ERequestType_Name(config.GetHandlers().GetRequestType());
        }
    }
    for (const auto& ref : ScenarioRegistry.GetScenarioRefs()) {
        if (ref->GetScenario().GetAcceptedFrames().empty() && !ref->GetScenario().AcceptsAnyUtterance()) {
            LOG_WARNING(globalCtx.BaseLogger()) << "Found useless scenario " << ref->GetScenario().GetName() << ": "
                                                << "accepts no frames and neither accepts any input.";
        }
    }
}

void TCommonScenarioWalker::MakeFakeScenarioResponse(const TRequest& request, TWalkerResponse& response,
                                                     const IContext& ctx) const {
    TScenarioResponse scenarioResponse(/* scenarioName= */ {}, /* scenarioSemanticFrames= */ {},
                                       /* scenarioAcceptsAnyUtterance= */ false);
    auto& builder = scenarioResponse.ForceBuilder(ctx.SpeechKitRequest(), request, GuidGenerator);
    builder.SetSession(/* dialogId= */ request.GetDialogId().GetOrElse(""),
                       ctx.SpeechKitRequest()->GetSession());
    response.AddScenarioResponse(std::move(scenarioResponse));
}

bool TCommonScenarioWalker::ProcessCallbackEvent(const TServerActionEvent& callback,
                                                 const IContext& ctx,
                                                 IFrameRequestProcessor& frameRequestProcessor,
                                                 const TMaybe<TIoTUserInfo>& iotUserInfo,
                                                 TRunState& runState,
                                                 NMegamind::TStackEngine& stackEngine,
                                                 std::unique_ptr<const IEvent>& event,
                                                 TEffectOptions& effectOptions,
                                                 TMaybe<TString>& callbackOwnerScenario,
                                                 NScenarios::TScenarioBaseRequest_ERequestSourceType& requestSource,
                                                 NMetrics::ISensors& sensors,
                                                 TRTLogger& logger) const {
    const bool isWarmUp = ctx.SpeechKitRequest().Event().GetIsWarmUp();
    switch (callback.GetCallbackType()) {
        case ECallbackType::SemanticFrame: {
            LOG_INFO(logger) << "Got SemanticFrame callback";
            frameRequestProcessor.Process(TTypedSemanticFrameRequest{callback.GetPayload()});
            break;
        }
        case ECallbackType::GetNext: {
            LOG_INFO(logger) << "Got GetNext callback";
            const auto onCallback = [&](const TEvent& skCallback) {
                LOG_INFO(logger) << "Event's been changed to: " << skCallback;
                event = IEvent::CreateEvent(skCallback);
            };
            TString scenarioName;
            if (const auto* scenarioNamePtr = MapFindPtr(callback.GetPayload().fields(),
                                                            TString{NMegamind::SCENARIO_NAME_JSON_KEY})) {
                scenarioName = scenarioNamePtr->string_value();
            } else {
                LOG_ERROR(ctx.Logger()) << "Callback directive doesn't have scenario_name in payload";
            }
            TString getNextStackSessionId;
            if (const auto* sessionId = MapFindPtr(callback.GetPayload().fields(), MM_STACK_ENGINE_SESSION_ID)) {
                getNextStackSessionId = sessionId->string_value();
            }
            if (const auto* parentProductScenario = MapFindPtr(callback.GetPayload().fields(),
                                                                MM_CALLBACK_PARENT_PRODUCT_SCENARIO_NAME)) {
                runState.AnalyticsInfoBuilder.SetParentProductScenarioName(parentProductScenario->string_value());
            }

            if (stackEngine.IsEmpty()) {
                sensors.IncRate(NSignal::LabelsForStackErrorsPerScenario(scenarioName, NSignal::STACK_ENGINE_EMPTY));
            }

            if (stackEngine.GetSessionId() != getNextStackSessionId) {
                sensors.IncRate(
                    NSignal::LabelsForStackErrorsPerScenario(scenarioName, NSignal::STACK_ENGINE_INVALID_SESSION));
                LOG_ERR(logger) << "Got invalid session id stack_session_id=" << getNextStackSessionId
                                << ". Current stack_session_id=" << stackEngine.GetSessionId();
            }

            if (!stackEngine.IsEmpty() && stackEngine.GetSessionId() != getNextStackSessionId && isWarmUp) {
                sensors.IncRate(
                    NSignal::LabelsForStackErrorsPerScenario(scenarioName, NSignal::STACK_ENGINE_INVALID_WARMUP));
            }

            if (stackEngine.IsEmpty() && ctx.HasExpFlag(EXP_ENABLE_STACK_ENGINE_MEMENTO_BACKUP)) {
                const auto* restoredStackEngine = ctx.MementoData().GetScenarioData(MM_STACK_ENGINE_MEMENTO_KEY);
                if (restoredStackEngine) {
                    LOG_WARN(logger) << "Got get_next with empty stack. Restoring stack from memento";
                    NMegamind::TStackEngineCore core;
                    restoredStackEngine->UnpackTo(&core);
                    LOG_INFO(logger) << "StackEngineCore: " << core;
                    stackEngine = NMegamind::TStackEngine{std::move(core)};
                    sensors.IncRate(NSignal::LabelsForStackRecoveriesPerScenario(
                        scenarioName, NSignal::STACK_RECOVERED_FROM_MEMENTO));
                }
            }

            if (!ctx.HasExpFlag(EXP_DISABLE_STACK_ENGINE_RECOVERY_CALLBACK) &&
                (stackEngine.IsEmpty() || stackEngine.GetSessionId() != getNextStackSessionId)) {
                const auto* recoveryCallback = MapFindPtr(callback.GetPayload().fields(),
                                                            MM_STACK_ENGINE_RECOVERY_CALLBACK_FIELD_NAME);
                if (recoveryCallback && recoveryCallback->has_struct_value()) {
                    if (isWarmUp) {
                        LOG_WARN(logger) << "Recovering stack on warm_up query";
                    }
                    LOG_WARN(logger) << "Got get_next with empty stack. Using recovery callback";
                    const auto skEvent = GetCallbackEventFromStruct(recoveryCallback->struct_value());
                    event = IEvent::CreateEvent(skEvent);
                    sensors.IncRate(
                        NSignal::LabelsForStackErrorsPerScenario(scenarioName, NSignal::STACK_ENGINE_RECOVERED));
                    sensors.IncRate(NSignal::LabelsForStackRecoveriesPerScenario(
                        scenarioName, NSignal::STACK_RECOVERED_FROM_CALLBACK));
                    break;
                }
            }
            if (stackEngine.GetSessionId() != getNextStackSessionId) {
                LOG_ERR(logger) << "Cannot process get_next with stack_session_id=" << getNextStackSessionId
                                << ". Current stack_session_id=" << stackEngine.GetSessionId();
                MakeFakeScenarioResponse(NMegamind::CreateRequest(std::move(event), ctx.SpeechKitRequest(),
                                                                    iotUserInfo, requestSource),
                                            runState.Response, ctx);
                PreFillAnalyticsInfo(runState.AnalyticsInfoBuilder, ctx);
                runState.Response.AnalyticsInfoBuilder = std::move(runState.AnalyticsInfoBuilder);
                return false;
            }

            const auto onGetNextRequestStatus =
                OnGetNextRequest(stackEngine, logger, frameRequestProcessor, onCallback, effectOptions,
                                    ctx.SpeechKitRequest(), sensors, iotUserInfo);
            if (onGetNextRequestStatus.Defined()) {
                runState.Response = TWalkerResponse{*onGetNextRequestStatus};
                return false;
            }
            callbackOwnerScenario = stackEngine.GetCore().GetStackOwner();
            requestSource = NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext;
            break;
        }
        default: {
            LOG_INFO(logger) << "Got click callback: " << callback.GetName();
            if (const auto* parentProductScenario = MapFindPtr(callback.GetPayload().fields(),
                                                                MM_CALLBACK_PARENT_PRODUCT_SCENARIO_NAME)) {
                runState.AnalyticsInfoBuilder.SetParentProductScenarioName(parentProductScenario->string_value());
            }
            MakeFakeScenarioResponse(NMegamind::CreateRequest(std::move(event), ctx.SpeechKitRequest(),
                                                                iotUserInfo, requestSource),
                                        runState.Response, ctx);

            PreFillAnalyticsInfo(runState.AnalyticsInfoBuilder, ctx);
            runState.Response.AnalyticsInfoBuilder = std::move(runState.AnalyticsInfoBuilder);
            return false;
        }
    }

    return true;
}

TMaybe<IScenarioWalker::TRequestState> TCommonScenarioWalker::RunPrepareRequest(IRunWalkerRequestCtx& walkerCtx, TRunState& runState) const {
    const auto& ctx = walkerCtx.Ctx();
    auto& logger = ctx.Logger();
    auto& sensors = ctx.Sensors();

    const bool isWarmUp = ctx.SpeechKitRequest().Event().GetIsWarmUp();
    std::unique_ptr<const IEvent> event = IEvent::CreateEvent(ctx.SpeechKitRequest().Event());
    if (!event) {
        runState.Response = TWalkerResponse{TError{TError::EType::Logic} << "Failed to parse request event."};
        return Nothing();
    }

    TVector<TSemanticFrame> forcedSemanticFrames;
    auto stackEngine = NMegamind::TStackEngine{ctx.StackEngineCore()};
    LOG_INFO(logger) << "StackEngineCore: " << stackEngine.GetCore();

    TEffectOptions effectOptions;
    auto requestSource = NScenarios::TScenarioBaseRequest_ERequestSourceType_Default;
    TVector<TSemanticFrame> recognizedActionEffectFrames;

    TMaybe<TIoTUserInfo> iotUserInfo;
    if (ctx.HasIoTUserInfo()) {
        iotUserInfo = ctx.IoTUserInfo();
    }

    TMaybe<TString> callbackOwnerScenario;

    NImpl::TFrameRequestProcessor frameRequestProcessor(
        recognizedActionEffectFrames,
        forcedSemanticFrames,
        event,
        logger,
        [&runState](const TString& productScenarioName){
            if (!productScenarioName.empty()) {
                runState.AnalyticsInfoBuilder.SetParentProductScenarioName(productScenarioName);
            }
        }
    );

    if (const auto* callback = event->AsServerActionEvent();
        callback && callback->GetCallbackType() != ECallbackType::None)
    {
        if (!ProcessCallbackEvent(*callback, ctx, frameRequestProcessor, iotUserInfo,
                                  runState, stackEngine, event, effectOptions, callbackOwnerScenario, requestSource,
                                  sensors, logger)) {
            return Nothing();
        }
    }

    if (const auto* callback = event->AsServerActionEvent()) {
        if (const auto* parentProductScenario = MapFindPtr(callback->GetPayload().fields(),
                                                           MM_CALLBACK_PARENT_PRODUCT_SCENARIO_NAME)) {
            runState.AnalyticsInfoBuilder.SetParentProductScenarioName(parentProductScenario->string_value());
        }
    }
    PreFillAnalyticsInfo(runState.AnalyticsInfoBuilder, ctx);

    bool utteranceWasUpdated = false;
    if (forcedSemanticFrames.empty()) {
        TActionEffect actionEffect = GetActionEffect(ctx, iotUserInfo, runState.AnalyticsInfoBuilder, runState.Response);
        if (!NImpl::ProcessActionEffect(actionEffect, runState, frameRequestProcessor,
            utteranceWasUpdated, event, recognizedActionEffectFrames, forcedSemanticFrames,
            logger, ctx.HasExpFlag(EXP_DO_NOT_FORCE_ACTION_EFFECT_FRAME_WHEN_NO_UTTERANCE_UPDATE))
        ) {
            return Nothing();
        }
    }

    auto enrichedFrames = EnrichSemanticFrames(ctx.Responses().WizardResponse().GetRequestFrames(recognizedActionEffectFrames));
    TVector<TSemanticFrame> allParsedSemanticFrames;
    if (ctx.HasExpFlag(EXP_PASS_ALL_PARSED_SEMANTIC_FRAMES)) {
        if (!utteranceWasUpdated) {
            allParsedSemanticFrames = enrichedFrames;
        }
        for (const auto& frame : forcedSemanticFrames) {
            allParsedSemanticFrames.push_back(frame);
        }
    }

    const TVector<TSemanticFrame>& requestFrames = forcedSemanticFrames.empty()
        ? enrichedFrames
        : forcedSemanticFrames;

    return TRequestState{CreateRequest(
        std::move(event), ctx.SpeechKitRequest(), ctx.Geobase(), iotUserInfo, requestSource, requestFrames,
        recognizedActionEffectFrames, std::move(stackEngine).ReleaseCore(), GetParameters(effectOptions),
        NImpl::GetContacts(ctx.SpeechKitRequest()), frameRequestProcessor.GetOrigin(), ctx.Session() ? ctx.Session()->GetLastWhisperTimeMs() : 0,
        ctx.GetWhisperTtlMs(), callbackOwnerScenario, ctx.GetTtsWhisperConfig(), ctx.Logger(), isWarmUp,
        allParsedSemanticFrames, frameRequestProcessor.GetDisableVoiceSession(), frameRequestProcessor.GetDisableShouldListen())};
}

TScenarioToRequestFrames TCommonScenarioWalker::ScenarioToRequestFrames(ILightWalkerRequestCtx& walkerCtx, const TRequest& request) const {
    auto& logger = walkerCtx.Ctx().Logger();

    // Check if there are scenario items in context and use it.
    THashMap<TString, NMegamindAppHost::TScenarioProto> scenariosProtos;
    auto onItem = [&scenariosProtos](NMegamindAppHost::TScenarioProto& proto) {
        scenariosProtos[proto.GetName()] = proto;
    };
    if (!walkerCtx.ItemProxyAdapter().ForEachCached<NMegamindAppHost::TScenarioProto>(NMegamind::AH_ITEM_SCENARIO, onItem)) {
        if (!scenariosProtos.empty()) {
            LOG_INFO(logger) << "Got scenarios from WALKER_PRECLASSIFY node";
            return MatchScenarios(scenariosProtos, ScenarioRegistry.GetScenarioRefs());
        }
    }

    auto framesToScenarios = AddDynamicAcceptedFrames(ScenarioRegistry.GetFramesToScenarios(), walkerCtx.Ctx().Session());
    FillAdditionalFrameSubscription(walkerCtx.Ctx(), framesToScenarios);
    if (framesToScenarios.empty()) {
        LOG_INFO(logger) << "Accepted frame to scenario mapping is empty";
    }
    const auto requestFrameToScenarioMatcher = CreateRequestFrameToScenarioMatcher(framesToScenarios);
    return requestFrameToScenarioMatcher->Match(request.GetSemanticFrames(), ScenarioRegistry.GetScenarioRefs());
}

TStatus TCommonScenarioWalker::TryAppHostPreClassify(IRunWalkerRequestCtx& walkerCtx,
                                                     TScenarioToRequestFrames& scenariosToRequestFrames,
                                                     TPreClassifyState& preClassifyState) const
{
    using namespace NMegamindAppHost;

    auto& itemAdapter = walkerCtx.ItemProxyAdapter();

    auto factorStorageResult = itemAdapter.GetFromContextCached<TFactorStorageBinaryProto>(NMegamind::AH_ITEM_FACTORSTORAGE_BINARY);
    auto qualityStorageResult = itemAdapter.GetFromContextCached<TQualityStorage>(NMegamind::AH_ITEM_QUALITYSTORAGE);
    auto preClassifyResult = itemAdapter.GetFromContextCached<TPreClassifyProto>(NMegamind::AH_ITEM_PRECLASSIFY);

    if (!preClassifyResult.IsSuccess()) {
        return std::move(*preClassifyResult.Error());
    }

    if (!qualityStorageResult.IsSuccess()) {
        return std::move(*qualityStorageResult.Error());
    }

    if (!factorStorageResult.IsSuccess()) {
        return std::move(*factorStorageResult.Error());
    }

    auto& logger = walkerCtx.Ctx().Logger();

    if (auto err = RestoreFactorStorage(walkerCtx, std::move(factorStorageResult))) {
        return std::move(*err);
    }
    RestorePreClassifyData(preClassifyState, std::move(preClassifyResult));
    RestoreQualityStorage(preClassifyState, std::move(qualityStorageResult));

    if (preClassifyState.IsTrashPartial) {
        return Success();
    }

    RestoreScenarios(GetScenarioRegistry(), itemAdapter, scenariosToRequestFrames);

    LOG_INFO(logger) << "Got preclassification from Prepare node";

    return Success();
}

TStatus TCommonScenarioWalker::RunPreClassify(IRunWalkerRequestCtx& walkerCtx,
                                              TRunState& runState, TRequestState& reqState,
                                              TMaybe<TPreClassifyState>& outState) const
{
    const auto& request = reqState.Request;
    const auto& ctx = walkerCtx.Ctx();
    const auto disableApply = ctx.HasExpFlag(EXP_DISABLE_APPLY);
    auto& logger = ctx.Logger();

    outState.ConstructInPlace(TPreClassifyState{
        .DisableApply = disableApply,
        .DeferredApplyMode = ctx.HasExpFlag(EXP_DONT_DEFER_APPLY) || disableApply
            ? EDeferredApplyMode::DontDeferApply
            : (request.IsWarmUp()
               ? EDeferredApplyMode::WarmUp
               : EDeferredApplyMode::DeferApply)
    });

    TScenarioToRequestFrames scenarioToRequestFrames;

    if (auto err = TryAppHostPreClassify(walkerCtx, scenarioToRequestFrames, *outState); err.Defined()) {
        auto& globalCtx = walkerCtx.GlobalCtx();

        scenarioToRequestFrames = ScenarioToRequestFrames(walkerCtx, request);

        if (outState->DeferredApplyMode == EDeferredApplyMode::WarmUp) {
            LOG_INFO(logger) << "Warming up request...";
        }

        LOG_WITH_TYPE(logger, TLOG_INFO, ELogMessageType::MegamindPreClasification)
            << "Begemot matched frames: " << GetFrameNameListString(request.GetSemanticFrames());

        // TODO(g-kostin,the0): move to pre.cpp
        if (scenarioToRequestFrames.empty()) {
            LOG_INFO(logger) << "Scenario to request frames mapping is empty";
        }
        for (const auto& [ref, frames] : scenarioToRequestFrames) {
            // On this stage we don't know whether scenario accepts any input or not. So
            // printing scenario names without frames is redundant. Moving this code to
            // pre.cpp would improve logging those scenarios.
            if (frames) {
                LOG_WITH_TYPE(logger, TLOG_INFO, ELogMessageType::MegamindPreClasification)
                    << "Scenario " << ref->GetScenario().GetName() << " accepts: " << GetFrameNameListString(frames);
            }
        }

        TFactorStorage& factorStorage = walkerCtx.FactorStorage();

        {
            THistogramScope scopedHistogram{
                walkerCtx.Ctx().Sensors(), NSignal::WALKER_STAGE_PRECLASSIFICATION,
                THistogramScope::ETimeUnit::Millis,
                /* aggregateLabelsBuilder= */ {},
                [&runState](const TDuration& duration) {
                    runState.AnalyticsInfoBuilder.SetPreClassifyDuration(duration.MicroSeconds());
                }};

            FillAsrFactors(request.GetEvent().SpeechKitEvent(), request, ctx.ExpFlags(), factorStorage);
            FillDeviceStateFactors(ctx.SpeechKitRequest(), factorStorage);
            FillSessionFactors(ctx.Session(), factorStorage);
            FillQueryFactors(ctx.Responses().WizardResponse(), factorStorage);
            FillScenarioQueryFactors(ctx.Responses().WizardResponse(), factorStorage);
            FillQueryTokensFactors(ctx.Responses().QueryTokensStatsResponse(), ctx.SpeechKitRequest().ClientFeatures(), factorStorage);
            FillNluFactors(ctx.Responses().WizardResponse(), factorStorage);

            outState->IsTrashPartial = PreClassifyPartial(ctx, factorStorage);
            if (!outState->IsTrashPartial) {
                PreClassify(scenarioToRequestFrames, request, ctx, globalCtx.GetFormulasStorage(), factorStorage, outState->QualityStorage);
            }
        }

        for (const auto& [ref, _] : scenarioToRequestFrames) {
            for (const auto& dataSource : ref->GetScenario().GetDataSources()) {
                walkerCtx.ItemProxyAdapter().AddFlag(TStringBuilder{} << "enable_" << NScenarios::GetDataSourceContextName(dataSource));
            }
        }
    }

    if (outState->IsTrashPartial) {
        return Success();
    }

    AskSkillProactivity(walkerCtx, request, scenarioToRequestFrames);

    AskWebSearch(request, walkerCtx, scenarioToRequestFrames);

    outState->ScenarioWrappers = NImpl::MakeScenarioWrappers(scenarioToRequestFrames, ctx, GuidGenerator,
                                                             outState->DeferredApplyMode, walkerCtx.ItemProxyAdapter());

    return Success();
}

void
TCommonScenarioWalker::RunScenarios(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, TRequestState& reqState,
                                    TPreClassifyState& preClassifyState) const {
    auto& response = runState.Response;

    if (!response.HasResponses()) {
        auto& ctx = walkerCtx.Ctx();
        const auto& requestProto = ctx.SpeechKitRequest()->GetRequest();
        const auto dialogHistory = ctx.Session() ? ctx.Session()->GetDialogHistory() : TDialogHistory{};
        const auto layout = ctx.Session() ? ctx.Session()->GetLayout() : Nothing();
        const auto& actions = ctx.Session() ? ctx.Session()->GetActions()
                                            : google::protobuf::Map<TString, NScenarios::TFrameAction>();

        const auto* ioTUserInfo = ctx.HasIoTUserInfo() ? &ctx.IoTUserInfo() : nullptr;

        TMaybe<NScenarios::TAppInfo> appInfo;
        if (requestProto.GetAdditionalOptions().HasAppInfo()) {
            appInfo.ConstructInPlace();
            appInfo->SetValue(requestProto.GetAdditionalOptions().GetAppInfo());
        }

        const auto* сurrentlyPlaying = requestProto.GetDeviceState().GetVideo().HasCurrentlyPlaying() ? &requestProto.GetDeviceState().GetVideo().GetCurrentlyPlaying() : nullptr;

        const TEnvironmentState* environmentState = nullptr;
        if (requestProto.HasEnvironmentState()) {
            environmentState = &requestProto.GetEnvironmentState();
        }
        TMaybe<TTandemEnvironmentState> tandemEnvironmentState;
        if (requestProto.HasEnvironmentState()) {
            tandemEnvironmentState.ConstructInPlace();
            const auto& environmentStateProto = requestProto.GetEnvironmentState();
            for (const auto& deviceProto: environmentStateProto.GetDevices()) {
                auto& device = *tandemEnvironmentState->AddDevices();
                device.MutableApplication()->CopyFrom(deviceProto.GetApplication());
                if (deviceProto.HasSpeakerDeviceState()) {
                    auto& tandemDeviceState = *device.MutableTandemDeviceState();
                    tandemDeviceState.MutableTandemState()->CopyFrom(deviceProto.GetSpeakerDeviceState().GetTandemState());
                    tandemDeviceState.MutableDeviceSubscriptionState()->CopyFrom(deviceProto.GetSpeakerDeviceState().GetDeviceSubscriptionState());
                }
            }
            for (const auto& groupProto: environmentStateProto.GetGroups()) {
                auto& group = *tandemEnvironmentState->AddGroups();
                group.CopyFrom(groupProto);
            }
        }
        const auto& request = reqState.Request;

        NMegamind::TDataSources dataSources(&ctx.Responses(), &request.GetUserLocation(),
                                            &dialogHistory, &actions, layout.Defined() ? layout.Get() : nullptr,
                                            &requestProto.GetSmartHomeInfo(),
                                            &requestProto.GetDeviceState().GetVideo().GetViewState(),
                                            &requestProto.GetNotificationState(),
                                            &ctx.Responses().SaasSkillDiscoveryResponse(),
                                            &requestProto.GetAdditionalOptions().GetQuasarAuxiliaryConfig(),
                                            ctx.Logger(),
                                            requestProto.GetDeviceState(),
                                            ioTUserInfo,
                                            appInfo.Defined() ? appInfo.Get() : nullptr,
                                            walkerCtx.ItemProxyAdapter(),
                                            requestProto.GetRawPersonalData(),
                                            сurrentlyPlaying,
                                            request.GetContactsList().Get(),
                                            environmentState,
                                            tandemEnvironmentState.Defined() ? tandemEnvironmentState.Get() : nullptr,
                                            GetWebSearchQuery(ctx, request.GetEvent()), request.GetWhisperInfo(),
                                            request.GetGuestData(), request.GetGuestOptions());

        InitScenarios(preClassifyState.ScenarioWrappers, request, ctx, walkerCtx.FactorStorage(),
                      walkerCtx.GlobalCtx().GetFormulasStorage(), dataSources, walkerCtx.RunStage(), response, walkerCtx.ItemProxyAdapter());
    }
}

void TCommonScenarioWalker::SendUdpSensors(const TVector<NAlice::TScenarioResponse>& responses,
                                           NMetrics::ISensors& sensors, bool isWinnerScenario) const {
    static constexpr TStringBuf emptyIntent = "NONE";
    for (const auto& scenario : responses) {
        const auto& intent = scenario.GetIntentFromFeatures();
        sensors.IncRate({
            {NSignal::UDP_SENSOR, NSignal::IS_WINNER_SCENARIO},
            {NSignal::INTENT, intent ? intent : emptyIntent},
            {NSignal::SCENARIO_NAME, scenario.GetScenarioName()},
            {NSignal::IS_WINNER_SCENARIO, ToString(isWinnerScenario)}
        });
        // process only first entry
        if (isWinnerScenario) {
            break;
        }
        if (scenario.GetFeatures().GetScenarioFeatures().GetIsIrrelevant()) {
            sensors.IncRate({
                {NSignal::UDP_SENSOR, NSignal::IS_IRRELEVANT},
                {NSignal::INTENT, intent ? intent : emptyIntent},
                {NSignal::SCENARIO_NAME, scenario.GetScenarioName()}
            });
        }
    }
}

void TCommonScenarioWalker::RunPostClassify(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                                            TPreClassifyState& preClassifyState) const {
    IGlobalCtx& globalCtx = walkerCtx.GlobalCtx();
    NMetrics::ISensors& sensors = walkerCtx.Ctx().Sensors();
    const auto& ctx = walkerCtx.Ctx();

    auto& qualityStorage = preClassifyState.QualityStorage;

    auto& response = runState.Response;
    const auto& request = reqState.Request;

    SendUdpSensors(response.Scenarios, sensors, /* isWinnerScenario= */ false);

    {
        THistogramScope scope{
            sensors, NSignal::WALKER_STAGE_POSTCLASSIFICATION,
            THistogramScope::ETimeUnit::Millis,
            /* aggregateLabelsBuilder= */ {},
            [&runState](const TDuration& duration) {
                runState.AnalyticsInfoBuilder.SetPostClassifyDuration(duration.MicroSeconds());
            }};

        auto& factorStorage = walkerCtx.FactorStorage();

        FillScenariosFactorStorage(response.Scenarios, factorStorage);
        TStatus error;
        const auto& webSearchResponse = ctx.Responses().WebSearchResponse(&error);
        if (!error) {
            FillSearchFactors(request.GetEvent().GetUtterance(), webSearchResponse, factorStorage);
        }
        FillNluFactors(ctx.Responses().WizardResponse(), factorStorage);

        PostClassify(
            ctx,
            request.GetInterfaces(),
            request.GetScenario(),
            request.GetRecognizedActionEffectFrames(),
            globalCtx.ScenarioConfigRegistry(),
            globalCtx.GetFormulasStorage(),
            factorStorage,
            response.Scenarios,
            qualityStorage
        );
        NMegamind::NFeatures::LogFeatures(ctx, factorStorage, qualityStorage);
    }

    response.QualityStorage = std::move(qualityStorage);
}

TScenarioWrapperPtr TCommonScenarioWalker::RunStartContinue(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                                            TPreClassifyState& preClassifyState) const
{
    const auto& ctx = walkerCtx.Ctx();
    NMetrics::ISensors& sensors = ctx.Sensors();

    auto& response = runState.Response;
    const auto& request = reqState.Request;

    TScenarioWrapperPtr wrapper;
    TScenarioResponse* topResponse = nullptr;
    auto& scenarioWrappers = preClassifyState.ScenarioWrappers;
    bool hasWinnerScenario = response.HasResponses();
    if (hasWinnerScenario) {
        topResponse = &response.Scenarios.front();

        SendUdpSensors(response.Scenarios, sensors, /* isWinnerScenario= */ true);

        wrapper = FindWrapper(*topResponse, scenarioWrappers);
        Y_ENSURE(wrapper, "The winner response should have a corresponding wrapper!");

        if (walkerCtx.ItemProxyAdapter().CheckFlagInInputContext(NMegamind::AH_FLAG_EXPECT_WEBSEARCH_RESPONSE)) {
            NImpl::AddSearchRelatedScenarioStats(ctx, wrapper, EHeavyScenarioEvent::Win);
        }
        ContinueHeavyScenario(wrapper, request, ctx, response);
    }
    return wrapper;
}

void TCommonScenarioWalker::RunFinishContinue(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                                             TPreClassifyState& preClassifyState) const
{
    auto& ctx = walkerCtx.Ctx();
    auto& logger = ctx.Logger();

    auto& response = runState.Response;
    const auto& request = reqState.Request;
    auto& analyticsInfoBuilder = runState.AnalyticsInfoBuilder;

    auto& postClassifyState = walkerCtx.PostClassifyState();

    bool hasWinnerScenario = response.HasResponses();
    TScenarioWrapperPtr wrapper = nullptr;
    TStatus continueError{};
    if (walkerCtx.RunStage() == ILightWalkerRequestCtx::ERunStage::ProcessContinue &&
        !walkerCtx.ItemProxyAdapter().CheckFlagInInputContext(EXP_DISABLE_APPHOST_CONTINUE_SCEANRIOS) &&
        !ctx.HasExpFlag(EXP_DISABLE_APPHOST_CONTINUE_SCEANRIOS))
    {
        const auto winnerOrError =
            RestorePostClassifyResults(postClassifyState, runState.Response, preClassifyState.ScenarioWrappers, ctx, request);
        hasWinnerScenario = winnerOrError.IsSuccess();
        wrapper = hasWinnerScenario ? winnerOrError.Value() : nullptr;
        continueError = winnerOrError.Status();
    }

    const auto postClassifyStatus = postClassifyState.GetPostClassifyStatus();

    TScenarioResponse* topResponse = nullptr;
    auto& scenarioWrappers = preClassifyState.ScenarioWrappers;
    if (hasWinnerScenario) {
        topResponse = &response.Scenarios.front();

        const TString& winnerName = topResponse->GetScenarioName();

        if (wrapper == nullptr) {
            wrapper = FindWrapper(*topResponse, scenarioWrappers);
        }
        Y_ENSURE(wrapper, "The winner response should have a corresponding wrapper!");

        if (const auto error = wrapper->FinishContinue(request, ctx, *topResponse)) {
            LOG_ERR(logger) << "The winner scenario " << winnerName
                            << " has failed its continue stage: " << error->ErrorMsg;
            hasWinnerScenario = false;
            continueError = TError{TError::EType::ScenarioError} << "Continue failed: " << error->ErrorMsg;
        }
    } else {
        MakeFakeScenarioResponse(request, response, ctx);
    }

    if (topResponse && topResponse->ResponseBodyIfExists()) {
        walkerCtx.ModifierRequestFactory().SetupModifierRequest(request, *topResponse->ResponseBodyIfExists(),
                                                                topResponse->GetScenarioName());
    } else {
        LOG_INFO(ctx.Logger()) << "Can't setup modifier request reason: "
                               << (topResponse ? "No top response" : "No response body");
    }

    TStatus postClassifyError;

    if (!hasWinnerScenario) {
        postClassifyError = continueError ? *continueError : TError{TError::EType::ScenarioError, TStringBuf("No winner scenario")};
    }

    if (!wrapper) {
        postClassifyError = TError{TError::EType::Logic, /* errorMsg= */ TStringBuf("No wrapper for topResponse")};
    }

    if (!postClassifyError.Defined()) {
        Y_ENSURE(topResponse, "Should have non-null winner response if any!");
    }

    THashSet<TString> scenariosWithTunnellerResponses = GetScenariosWithTunnellerResponses(ctx);

    SavePostClassifyState(walkerCtx, ctx, analyticsInfoBuilder, scenarioWrappers, scenariosWithTunnellerResponses,
                          request, response, postClassifyError, wrapper);
}

TErrorOr<TScenarioWrapperPtr>
TCommonScenarioWalker::RestorePostClassifyResults(NMegamind::IPostClassifyState& postClassifyState,
                                                  TWalkerResponse& walkerResponse, TScenarioWrapperPtrs wrappers,
                                                  IContext& ctx, const TRequest& request) const {
    if (auto error = RestoreQualityStorage(walkerResponse, postClassifyState)) {
        return std::move(*error);
    }
    if (auto error = RestoreAnalytics(walkerResponse, postClassifyState)) {
        return std::move(*error);
    }
    RestoreScenarioErrors(walkerResponse, postClassifyState);

    auto winnerOrError = postClassifyState.GetWinnerScenario();
    if (winnerOrError.Error()) {
        return std::move(*winnerOrError.Error());
    }

    return NImpl::RestoreWinner(walkerResponse, winnerOrError.Value(), wrappers, ctx, request,
                                postClassifyState.GetContinueResponse());
}

TErrorOr<TScenarioWrapperPtr> TCommonScenarioWalker::RestorePostClassifyAndContinueResults(
    NMegamind::IPostClassifyState& postClassifyState, TWalkerResponse& walkerResponse, TScenarioWrapperPtrs wrappers,
    IContext& ctx, const TRequest& request, NMegamind::TItemProxyAdapter& itemAdapter) const {
    auto winner = RestorePostClassifyResults(postClassifyState, walkerResponse, wrappers, ctx, request);
    if (winner.Error()) {
        return std::move(*winner.Error());
    }
    if (auto status =
            winner.Value()->RestoreInit(itemAdapter);
        status.Defined()) {
        return std::move(*status);
    }
    return winner;
}

TWalkerResponse
TCommonScenarioWalker::RunFinalizeCombinator(IRunWalkerRequestCtx& walkerCtx, TPreClassifyState& preClassifyState,
                                             const TRequest& request, const TCombinatorConfig& combinatorConfig,
                                             NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsInfoBuilder,
                                             NMegamindAppHost::TCombinatorProto::ECombinatorStage stage) const {
    auto responseProto = TryChooseCombinator(walkerCtx, combinatorConfig, stage);
    if (!responseProto.Defined()) {
        return TWalkerResponse{TError{TError::EType::Logic} << "Can't choose combinator in finalize: "
                                                            << combinatorConfig.GetName()};
    }
    NMegamind::TCombinatorResponse combinatorResponse{combinatorConfig};
    combinatorResponse.SetResponseProto(std::move(*responseProto));
    TWalkerResponse walkerResponse{};
    if (auto error =
            RenderCombinatorResponse(walkerCtx, walkerResponse, std::move(analyticsInfoBuilder),
                                     request, preClassifyState, combinatorResponse)) {
        LOG_ERR(walkerCtx.Ctx().Logger()) << "Render combinator error: " << *error;
    }
    return walkerResponse;
}

TWalkerResponse
TCommonScenarioWalker::RunFinalizeScenario(IRunWalkerRequestCtx& walkerCtx, TPreClassifyState& preClassifyState,
                                           const TRequest& request, NMegamind::IPostClassifyState& postClassifyState,
                                           NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsFromPrepare) const {
    auto& ctx = walkerCtx.Ctx();
    TWalkerResponse walkerResponse{};
    walkerResponse.AnalyticsInfoBuilder = std::move(analyticsFromPrepare);
    const auto winnerOrError =
        RestorePostClassifyAndContinueResults(postClassifyState, walkerResponse, preClassifyState.ScenarioWrappers,
                                              ctx, request, walkerCtx.ItemProxyAdapter());
    const auto postClassifyStatus = postClassifyState.GetPostClassifyStatus();

    if (walkerResponse.Scenarios.empty()) {
        MakeFakeScenarioResponse(request, walkerResponse, ctx);
    }

    const auto scenariosWithTunnellerResponses =
        NImpl::GetScenariosFromFlag(ctx.ExpFlag(EXP_MOVE_TUNNELLER_RESPONSES_FROM_SCENARIOS));

    if (winnerOrError.Error()) {
        LOG_ERROR(ctx.Logger()) << "AppHost PostClassify error: " << *winnerOrError.Error();
        OnNoWinnerScenarioResponse(request, walkerResponse, /* wrappers= */ {},
                                   std::move(walkerResponse.AnalyticsInfoBuilder), walkerCtx,
                                   scenariosWithTunnellerResponses,
                                   postClassifyStatus.Defined() ? *postClassifyStatus : *winnerOrError.Error());
        return walkerResponse;
    }
    auto winnerScenario = winnerOrError.Value();

    if (postClassifyStatus.Defined()) {
        OnNoWinnerScenarioResponse(request, walkerResponse, {winnerScenario},
                                   std::move(walkerResponse.AnalyticsInfoBuilder), walkerCtx,
                                   scenariosWithTunnellerResponses, *postClassifyStatus);
        return walkerResponse;
    }

    auto& scenarioResponse = walkerResponse.Scenarios.front();

    auto proactivityAnswer =
        WaitProactivity(walkerCtx.ItemProxyAdapter(), ctx, scenarioResponse);

    const auto postAnalyticsFiller = [&](TScenarioWrapperPtr wrapperPtr) {
        PostFillAnalyticsInfo(walkerResponse.AnalyticsInfoBuilder, ctx, /* scenarios= */ {wrapperPtr}, wrapperPtr,
                              scenariosWithTunnellerResponses, request);
        NImpl::CopyScenarioErrorsToWinnerResponse(walkerResponse, scenarioResponse, ctx);
    };

    const auto result = TryApplyAndFinalizeOrRenderError(
        preClassifyState.DisableApply, scenarioResponse, winnerScenario, request, walkerCtx, ctx.Logger(),
        ctx.Language(), walkerResponse.AnalyticsInfoBuilder, walkerResponse.ProactivityLogStorage, walkerResponse.QualityStorage,
        proactivityAnswer, ECalledFrom::RunStage, winnerScenario->GetSemanticFrames(), postAnalyticsFiller);

    if (result.Error()) {
        LOG_ERROR(ctx.Logger()) << "TryApplyAndFinalizeOrRenderError error: " << *result.Error();
    }

    CheckFinalResponses(ctx, scenarioResponse, walkerResponse);

    return walkerResponse;
}

TWalkerResponse
TCommonScenarioWalker::RunFinalize(IRunWalkerRequestCtx& walkerCtx, TPreClassifyState& preClassifyState,
                                   const TRequest& request,
                                   NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsFromPrepare) const {
    auto& postClassifyState = walkerCtx.PostClassifyState();
    auto winnerCombinator = postClassifyState.GetWinnerCombinator();
    auto stage = postClassifyState.GetWinnerCombinatorStage();

    if (winnerCombinator.Defined() && stage.Defined()) {
        return RunFinalizeCombinator(
            walkerCtx, preClassifyState, request,
            walkerCtx.GlobalCtx().CombinatorConfigRegistry().GetCombinatorConfig(winnerCombinator.GetRef()),
            std::move(analyticsFromPrepare), stage.GetRef());
    }
    return RunFinalizeScenario(walkerCtx, preClassifyState, request, postClassifyState,
                               std::move(analyticsFromPrepare));
}

void TCommonScenarioWalker::OnNoWinnerScenarioResponse(const TRequest& request, TWalkerResponse& response,
                                                       const TScenarioWrapperPtrs& scenarioWrappers,
                                                       NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsInfoBuilder,
                                                       ILightWalkerRequestCtx& walkerCtx, const THashSet<TString>& scenariosWithTunnellerResponses,
                                                       const TError& error) const {
    auto& ctx = walkerCtx.Ctx();
    auto& logger = ctx.Logger();
    if (response.Scenarios.empty()) {
        LOG_ERR(logger) << "Scenarios should not be empty";
        return;
    }
    auto& badWinnerResponse = response.Scenarios.front();
    if (!TryRenderError(request, error, ctx.Language(), badWinnerResponse, walkerCtx, logger)) {
        LOG_ERR(logger) << "Failed to render error NLG for all scenario failed";
    }

    NImpl::CopyScenarioErrorsToWinnerResponse(response, badWinnerResponse, ctx);
    PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, scenarioWrappers, /* wrapper= */ nullptr,
                          scenariosWithTunnellerResponses, request);
    response.AnalyticsInfoBuilder = std::move(analyticsInfoBuilder);
}

TMaybe<NScenarios::TCombinatorResponse>
TCommonScenarioWalker::TryChooseCombinator(IRunWalkerRequestCtx& walkerCtx,
                                           const TCombinatorConfig& combinatorConfig,
                                           NMegamindAppHost::TCombinatorProto::ECombinatorStage stage) const {
    const auto& ctx = walkerCtx.Ctx();
    auto& logger = ctx.Logger();

    const auto& combinatorName = combinatorConfig.GetName();
    auto combinatorItemName = TString{NMegamind::AH_ITEM_COMBINATOR_RESPONSE_PREFIX} + combinatorName;
    if (stage == NMegamindAppHost::TCombinatorProto::Continue) {
        combinatorItemName = TString{NMegamind::AH_ITEM_COMBINATOR_CONTINUE_RESPONSE_PREFIX} + combinatorName;
    }
    auto combinatorResponseOrError =
        walkerCtx.ItemProxyAdapter().GetFromContext<NScenarios::TCombinatorResponse>(combinatorItemName);
    if (combinatorResponseOrError.Error()) {
        LOG_ERR(logger) << "Combinator " << combinatorName
                        << " failed to respond: " << *combinatorResponseOrError.Error();
        return Nothing();
    }

    auto& combinatorResponse = combinatorResponseOrError.Value();
    if (combinatorResponse.GetResponse().GetFeatures().GetIsIrrelevant()) {
        LOG_INFO(logger) << "Combinator " << combinatorName << " is irrelevant by features";
        ctx.Sensors().IncRate(NSignal::LabelsForIrrelevantCombinator(combinatorName));
        return Nothing();
    }
    return std::move(combinatorResponse);
}

TStatus
TCommonScenarioWalker::RenderCombinatorResponse(IRunWalkerRequestCtx& walkerCtx, TWalkerResponse& walkerResponse,
                                                NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsInfoBuilder,
                                                const TRequest& request, TPreClassifyState& preClassifyState,
                                                const NMegamind::TCombinatorResponse& combinatorResponse) const {
    const auto& ctx = walkerCtx.Ctx();

    const auto& combinatorConfig = combinatorResponse.GetConfig();

    if (combinatorResponse.ResponseProtoIfExists() == nullptr) {
        return TError{TError::EType::Logic} << "Don't have response proto in combinator response";
    }

    const auto& responseProto = *combinatorResponse.ResponseProtoIfExists();

    TScenarioResponse scenarioResponse(/* scenarioName= */ combinatorConfig.GetName(),
                                       /* scenarioSemanticFrames= */ {},
                                       /* scenarioAcceptsAnyUtterance= */ combinatorConfig.GetAcceptsAllFrames());
    NMegamind::TMegamindAnalyticsInfo megamindAnalyticsInfo;
    const auto* session = ctx.Session();
    if (session && session->GetPreviousScenarioName() == combinatorConfig.GetName()) {
        if (const auto analyticsInfo = session->GetMegamindAnalyticsInfo()) {
            megamindAnalyticsInfo = *analyticsInfo;
        }
    }

    NMegamind::TAnalyticsInfoBuilder analyticsInfo;

    analyticsInfoBuilder.SetCombinatorAnalyticsInfo(responseProto.GetCombinatorsAnalyticsInfo());

    scenarioResponse.SetResponseBody(responseProto.GetResponse().GetResponseBody());

    if (auto error = walkerCtx.ModifierRequestFactory().ApplyModifierResponse(scenarioResponse, analyticsInfoBuilder)) {
        return std::move(*error);
    }

    const auto* responseBody = scenarioResponse.ResponseBodyIfExists();
    if (!responseBody) {
        return TError{TError::EType::Logic} << "Empty response_body after modifier";
    }

    if (const auto textResponse = ParseTextResponse(*responseBody, scenarioResponse);
        !textResponse.GetText().empty()) {
        walkerCtx.ItemProxyAdapter().PutIntoContext(textResponse, NMegamind::AH_ITEM_SCENARIOS_RESPONSE_MONITORING);
    }
    BuildScenarioResponseFromResponseBody(*responseBody, scenarioResponse, ctx, request, GuidGenerator,
                                          megamindAnalyticsInfo, analyticsInfo, VERSION_STRING);

    auto& factorStorage = walkerCtx.FactorStorage();
    FillScenariosFactorStorage(walkerResponse.Scenarios, factorStorage);
    TStatus error;
    const auto& webSearchResponse = ctx.Responses().WebSearchResponse(&error);
    if (!error) {
        FillSearchFactors(request.GetEvent().GetUtterance(), webSearchResponse, factorStorage);
    }

    // TODO PostFillAnalyticsInfo with winner combinator
    THashSet<TString> scenariosWithTunnellerResponses =
        NImpl::GetScenariosFromFlag(ctx.ExpFlag(EXP_MOVE_TUNNELLER_RESPONSES_FROM_SCENARIOS));

    NImpl::CopyScenarioErrorsToWinnerResponse(walkerResponse, scenarioResponse, ctx);

    auto proactivity = WaitProactivity(walkerCtx.ItemProxyAdapter(), ctx, scenarioResponse);

    // TODO AddSearchRelatedScenarioStats for used scenarios or smth
    if (auto* builder = scenarioResponse.BuilderIfExists()) {
        NMegamind::FinalizeCombinator(*builder, walkerCtx, request, responseProto, preClassifyState,
                                      combinatorConfig.GetName());
    } else {
        return TError{TError::EType::Logic} << "Empty builder to finalize combinator";
    }

    TVector<TScenarioResponse> resultResponses;
    resultResponses.push_back(std::move(scenarioResponse));
    ctx.Sensors().IncRate(NSignal::LabelsForWinningCombinator(combinatorConfig.GetName()));
    for (auto& resp : walkerResponse.Scenarios) {
        if (FindIfPtr(responseProto.GetUsedScenarios(),
                      [&resp](const TString& name) { return name == resp.GetScenarioName(); })) {
            if (const auto* respBody = resp.ResponseBodyIfExists()) {
                if (const auto wrapper = FindWrapper(resp, preClassifyState.ScenarioWrappers)) {
                    auto& scenarioAnalyticsInfo = wrapper->GetAnalyticsInfo();
                    BuildScenarioResponseFromResponseBody(*respBody, resp, ctx, request, GuidGenerator,
                                                          megamindAnalyticsInfo, scenarioAnalyticsInfo,
                                                          VERSION_STRING);
                    analyticsInfoBuilder.AddAnalyticsInfo(resp.GetScenarioName(), scenarioAnalyticsInfo.Build());
                }
            }
            ctx.Sensors().IncRate(NSignal::LabelsForWinningScenarioInCombinator(
                combinatorConfig.GetName(), resp.GetScenarioName()));
            resultResponses.push_back(std::move(resp));
        }
    }
    walkerResponse.Scenarios = std::move(resultResponses);

    PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, preClassifyState.ScenarioWrappers,
                          /* winnerScenario= */ nullptr, scenariosWithTunnellerResponses, request);

    CheckFinalResponses(ctx, scenarioResponse, walkerResponse);

    walkerResponse.AnalyticsInfoBuilder = std::move(analyticsInfoBuilder);
    return Success();
}

TMaybe<NMegamind::TCombinatorResponse>
TCommonScenarioWalker::CheckCombinatorResponse(IRunWalkerRequestCtx& walkerCtx) const {
    auto& ctx = walkerCtx.Ctx();

    auto& itemProxyAdapter = walkerCtx.ItemProxyAdapter();
    const auto errorOrLaunchedCombinators =
        itemProxyAdapter.GetFromContext<NMegamindAppHost::TLaunchedCombinators>(NMegamind::AH_ITEM_LAUNCHED_COMBINATORS);
    if (errorOrLaunchedCombinators.Error()) {
        LOG_ERR(walkerCtx.Ctx().Logger()) << "Failed to get launched combinators list: " << *errorOrLaunchedCombinators.Error();
        return Nothing();
    }

    const auto& launchedCombinators = errorOrLaunchedCombinators.Value().GetCombinators();
    auto wasLaunched = [&launchedCombinators](const TString& combinatorName) {
        const auto* launchedCombinatorInfo =
            FindIfPtr(launchedCombinators, [&combinatorName](const NMegamindAppHost::TLaunchedCombinators::TLaunchedCombinatorInfo& combinatorInfo) {
                return combinatorInfo.GetName() == combinatorName;
            });
        return launchedCombinatorInfo != nullptr;
    };

    for (const auto& [_, config] : walkerCtx.GlobalCtx().CombinatorConfigRegistry().GetCombinatorConfigs()) {
        if (!wasLaunched(config.GetName())) {
            continue;
        }
        if (auto responseProto = TryChooseCombinator(walkerCtx, config, NMegamindAppHost::TCombinatorProto::Run); responseProto.Defined()) {
            LOG_INFO(ctx.Logger()) << "Successfully chosen combinator " << config.GetName() << " response";
            NMegamind::TCombinatorResponse response{config};
            response.SetResponseProto(*responseProto);
            return std::move(response);
        }
    }
    return Nothing();
}


void TCommonScenarioWalker::RunClassifyWinner(IRunWalkerRequestCtx& walkerCtx, const TRequest& requestModel) const {
    if (const auto combinatorResponse = CheckCombinatorResponse(walkerCtx); combinatorResponse.Defined()) {
        walkerCtx.SaveCombinatorState(*combinatorResponse, requestModel);
        walkerCtx.ItemProxyAdapter().AddFlag("combinator_winner_case");
        // TODO use conbinator->UsedScenarios
        AddStartScenarioContinueFlag(HOLLYWOOD_MUSIC_SCENARIO, walkerCtx.ItemProxyAdapter());
    } else {
        walkerCtx.ItemProxyAdapter().AddFlag("scenario_winner_case");
    }
}

void TCommonScenarioWalker::RunProcessCombinatorContinue(IRunWalkerRequestCtx& walkerCtx, const TRequest& requestModel) const {
    auto& postClassifyState = walkerCtx.PostClassifyState();
    auto winnerCombinator = postClassifyState.GetWinnerCombinator();
    auto stage = postClassifyState.GetWinnerCombinatorStage();

    if (winnerCombinator.Defined() && stage.Defined()) {
        auto& config = walkerCtx.GlobalCtx().CombinatorConfigRegistry().GetCombinatorConfig(winnerCombinator.GetRef());
        auto responseProto = TryChooseCombinator(walkerCtx, config, stage.GetRef());
        if (responseProto && responseProto->HasResponse() && responseProto->GetResponse().HasResponseBody()) {
            walkerCtx.ModifierRequestFactory().SetupModifierRequest(
                requestModel, responseProto->GetResponse().GetResponseBody(), config.GetName());
        }
    }
}

TWalkerResponse TCommonScenarioWalker::RunFinalizeStage(IRunWalkerRequestCtx& walkerCtx) const {
    auto& requestCtx = walkerCtx.RequestCtx();

    TRunState runState;
    TMaybe<TRequestState> requestState;
    TMaybe<TPreClassifyState> preClassifyState;
    if (const auto err = RunPrepare(walkerCtx, runState, requestState, preClassifyState)) {
        LOG_ERR(walkerCtx.Ctx().Logger()) << "RunPrepare: " << err;
        return std::move(runState.Response);
    }
    Y_ASSERT(requestState.Defined() && preClassifyState.Defined());

    auto walkerResponse =
        RunFinalize(walkerCtx, *preClassifyState, requestState->Request, std::move(runState.AnalyticsInfoBuilder));
    requestCtx.StageTimers().RegisterAndSignal(requestCtx, NMegamind::TS_STAGE_WALKER_AFTER_FINALIZE,
                                               NMegamind::TS_STAGE_START_REQUEST, walkerCtx.Ctx().Sensors());
    return walkerResponse;
}

TWalkerResponse TCommonScenarioWalker::RunProcessContinueStage(IRunWalkerRequestCtx& walkerCtx) const {
    auto& requestCtx = walkerCtx.RequestCtx();

    TRunState runState;
    TMaybe<TRequestState> requestState;
    TMaybe<TPreClassifyState> preClassifyState;
    if (const auto err = RunPrepare(walkerCtx, runState, requestState, preClassifyState)) {
        LOG_ERR(walkerCtx.Ctx().Logger()) << "RunPrepare: " << err;
        return std::move(runState.Response);
    }
    Y_ASSERT(requestState.Defined() && preClassifyState.Defined());

    RunFinishContinue(walkerCtx, runState, *requestState, *preClassifyState);
    requestCtx.StageTimers().RegisterAndSignal(requestCtx, NMegamind::TS_STAGE_WALKER_AFTER_PROCESS_CONTINUE,
                                               NMegamind::TS_STAGE_START_REQUEST, walkerCtx.Ctx().Sensors());
    return std::move(runState.Response);;
}

TWalkerResponse TCommonScenarioWalker::RunPreClassifyStage(IRunWalkerRequestCtx& walkerCtx) const {
    auto& requestCtx = walkerCtx.RequestCtx();

    TRunState runState;
    TMaybe<TRequestState> requestState;
    TMaybe<TPreClassifyState> preClassifyState;
    if (const auto err = RunPrepare(walkerCtx, runState, requestState, preClassifyState)) {
        LOG_ERR(walkerCtx.Ctx().Logger()) << "RunPrepare: " << err;
        return std::move(runState.Response);
    }
    Y_ASSERT(requestState.Defined() && preClassifyState.Defined());

    NImpl::PushFlagsForConditionalDatasources(walkerCtx.ItemProxyAdapter(), walkerCtx.Ctx(), preClassifyState->ScenarioWrappers);
    RunScenarios(walkerCtx, runState, *requestState, *preClassifyState);

    requestCtx.StageTimers().RegisterAndSignal(requestCtx, NMegamind::TS_STAGE_WALKER_AFTER_SCENARIOS,
                                               NMegamind::TS_STAGE_START_REQUEST, walkerCtx.Ctx().Sensors());

    return TWalkerResponse{TError{TError::EType::Logic}};
}

TStatus TCommonScenarioWalker::AskScenarios(TScenarioWrapperPtrs& wrappers, const TRequest& request,
                                            const IContext& ctx, const TFactorStorage& factorStorage,
                                            const TFormulasStorage& formulasStorage, TWalkerResponse& response,
                                            NMegamind::TItemProxyAdapter& itemProxyAdapter) const {
    auto initializedWrappers = NImpl::RestoreInitializedWrappers(itemProxyAdapter, wrappers);
    if (initializedWrappers.Error()) {
        return std::move(*initializedWrappers.Error());
    }

    OnlyFor<TConfigBasedAppHostProxyProtocolScenario, TConfigBasedAppHostPureProtocolScenario>(
        initializedWrappers.Value(), [&, this](const TScenario& scenario, TScenarioWrapperPtr wrapper) {
            AskScenario(request, ctx, factorStorage, formulasStorage, scenario, wrapper, response);
        });
    return Success();
}

TWalkerResponse TCommonScenarioWalker::RunPostClassifyStage(IRunWalkerRequestCtx& walkerCtx) const {
    auto& ctx = walkerCtx.Ctx();
    auto& requestCtx = walkerCtx.RequestCtx();

    TRunState runState;
    TMaybe<TRequestState> requestState;
    TMaybe<TPreClassifyState> preClassifyState;
    if (const auto err = RunPrepare(walkerCtx, runState, requestState, preClassifyState)) {
        LOG_ERR(walkerCtx.Ctx().Logger()) << "RunPrepare: " << err;
        return std::move(runState.Response);
    }
    Y_ASSERT(requestState.Defined() && preClassifyState.Defined());

    AskScenarios(preClassifyState->ScenarioWrappers, requestState->Request, ctx, walkerCtx.FactorStorage(),
                 walkerCtx.GlobalCtx().GetFormulasStorage(), runState.Response, walkerCtx.ItemProxyAdapter());

    requestCtx.StageTimers().RegisterAndSignal(requestCtx, NMegamind::TS_STAGE_WALKER_AFTER_SCENARIOS,
                                               NMegamind::TS_STAGE_START_REQUEST, walkerCtx.Ctx().Sensors());

    if (const auto combinatorResponse = CheckCombinatorResponse(walkerCtx); combinatorResponse.Defined()) {
        walkerCtx.SaveCombinatorState(*combinatorResponse, requestState->Request);
    } else {
        LOG_INFO(ctx.Logger()) << "No combinator response. Using PostClassify";
        RunPostClassify(walkerCtx, runState, *requestState, *preClassifyState);
        const TScenarioWrapperPtr wrapper = RunStartContinue(walkerCtx, runState, *requestState, *preClassifyState);
        if (!walkerCtx.ItemProxyAdapter().CheckFlagInInputContext(EXP_DISABLE_APPHOST_CONTINUE_SCEANRIOS) &&
            !ctx.HasExpFlag(EXP_DISABLE_APPHOST_CONTINUE_SCEANRIOS))
        {
            THashSet<TString> scenariosWithTunnellerResponses = GetScenariosWithTunnellerResponses(ctx);
            SavePostClassifyState(walkerCtx, ctx, runState.AnalyticsInfoBuilder, preClassifyState->ScenarioWrappers, scenariosWithTunnellerResponses,
                                  requestState->Request, runState.Response, Success(), wrapper);
        } else {
            RunFinishContinue(walkerCtx, runState, *requestState, *preClassifyState);
        }
    }
    requestCtx.StageTimers().RegisterAndSignal(requestCtx, NMegamind::TS_STAGE_WALKER_AFTER_POSTCLASSIFY,
                                               NMegamind::TS_STAGE_START_REQUEST, walkerCtx.Ctx().Sensors());
    return std::move(runState.Response);
}

TStatus TCommonScenarioWalker::ModifyApplyScenarioResponse(ILightWalkerRequestCtx& walkerCtx,
                                                           TErrorOr<TApplyState>&& applyStateOrError) const {
    if (auto* error = applyStateOrError.Error(); error) {
        return std::move(*error);
    }

    auto& applyState = applyStateOrError.Value();
    if (applyState.ActionEffect.Status == TActionEffect::EStatus::WalkerResponseIsComplete) {
        return Success();
    }

    auto& response = applyState.WalkerResponse;
    auto& wrapper = applyState.ScenarioWrapper;
    if (!wrapper) {
        return TError{TError::EType::Logic} << "No wrapper after Restore Apply State";
    }
    if (response.Scenarios.empty()) {
        return TError{TError::EType::Logic} << "No scenario response after Restore Apply State";
    }
    if (!applyState.Request.Defined()) {
        return TError{TError::EType::Logic} << "No request defined after Restore Apply State";
    }

    auto& scenarioResponse = response.Scenarios.front();
    const auto& request = applyState.Request.GetRef();

    const auto& ctx = walkerCtx.Ctx();

    const auto applyResult = wrapper->FinishApply(request, ctx, scenarioResponse);
    if(applyResult.Error()) {
        return *applyResult.Error();
    }

    if (applyResult.Value() != EApplyResult::Called) {
        return TError{TError::EType::Logic} << "Apply result should be called in apply stage";
    }

    if (!scenarioResponse.ResponseBodyIfExists()) {
        return TError{TError::EType::Logic} << "No response body after Finish Apply";
    }

    walkerCtx.ModifierRequestFactory().SetupModifierRequest(request, *scenarioResponse.ResponseBodyIfExists(),
                                                            scenarioResponse.GetScenarioName());

    return Success();
}

TErrorOr<IScenarioWalker::TApplyState>
TCommonScenarioWalker::RestoreApplyState(ILightWalkerRequestCtx& walkerCtx) const {
    const auto& ctx = walkerCtx.Ctx();

    const auto* session = ctx.Session();
    if (!session) {
        return TError{TError::EType::Logic} << "No session during Apply() invocation";
    }

    const TString scenarioName = session->GetPreviousScenarioName();

    const auto* ref = FindIfPtr(ScenarioRegistry.GetScenarioRefs(), [&scenarioName](const auto& ref) {
        return ref->GetScenario().GetName() == scenarioName;
    });
    if (!ref) {
        return TError(TError::EType::Logic)
                               << "Can't find scenario " << scenarioName.Quote() << " to call Apply()";
    }

    const TVector<TSemanticFrame> noFrames;
    TApplyState applyState;
    auto& wrapper = applyState.ScenarioWrapper;

    OnlyVisit<TConfigBasedAppHostProxyProtocolScenario>(*ref, [&](const TConfigBasedAppHostProxyProtocolScenario& scenario) {
        wrapper = MakeIntrusive<TAppHostProxyProtocolScenarioWrapper>(scenario, ctx, noFrames,
                                                                      GuidGenerator,
                                                                      EDeferredApplyMode::DeferredCall,
                                                                      /* restoreAllFromSession= */ true,
                                                                      walkerCtx.ItemProxyAdapter());
    });
    OnlyVisit<TConfigBasedAppHostPureProtocolScenario>(*ref, [&](const TConfigBasedAppHostPureProtocolScenario& scenario) {
        wrapper = MakeIntrusive<TAppHostPureProtocolScenarioWrapper>(scenario, ctx, noFrames,
                                                                     GuidGenerator,
                                                                     EDeferredApplyMode::DeferredCall,
                                                                     /* restoreAllFromSession= */ true,
                                                                     walkerCtx.ItemProxyAdapter());
    });

    auto& response = applyState.WalkerResponse;
    if (wrapper) {
        response.AnalyticsInfoBuilder.CopyFromProto(wrapper->GetMegamindAnalyticsInfo());
        response.QualityStorage = std::move(wrapper->GetQualityStorage());
    }

    TMaybe<TIoTUserInfo> iotUserInfo;
    if (ctx.HasIoTUserInfo()) {
        iotUserInfo = ctx.IoTUserInfo();
    }

    applyState.ActionEffect = GetActionEffect(ctx, iotUserInfo, response.AnalyticsInfoBuilder, response);
    auto& actionEffect = applyState.ActionEffect;
    if (actionEffect.Status == TActionEffect::EStatus::WalkerResponseIsComplete) {
        return applyState;
    }
    std::unique_ptr<const IEvent> event = IEvent::CreateEvent(ctx.SpeechKitRequest().Event());
    if (actionEffect.UpdatedEvent) {
        event = std::move(actionEffect.UpdatedEvent);
    }
    if (!event) {
        return TError{TError::EType::Logic} << "Failed to parse request event.";
    }

    TMaybe<TOrigin> origin;
    if (const auto* callback = event->AsServerActionEvent();
        callback && callback->GetCallbackType() == ECallbackType::SemanticFrame)
    {
        origin = TTypedSemanticFrameRequest{callback->GetPayload()}.Origin;
    }

    const auto& whisperConfig = ctx.GetTtsWhisperConfig();

    applyState.Request =
        NMegamind::CreateRequest(std::move(event), ctx.SpeechKitRequest(),
                                 ctx.Geobase(),
                                 iotUserInfo,
                                 NScenarios::TScenarioBaseRequest_ERequestSourceType_Default,
                                 /* semanticFrames= */ {},
                                 /* recognizedActionEffectFrames= */ {}, ctx.StackEngineCore(),
                                 /* parameters= */ {}, NImpl::GetContacts(ctx.SpeechKitRequest()), origin,
                                 session->GetLastWhisperTimeMs(), ctx.GetWhisperTtlMs(),
                                 /* callbackOwnerScenario= */ Nothing(), whisperConfig, ctx.Logger());

    TScenarioResponse scenarioResponse{scenarioName,
                               wrapper ? wrapper->GetSemanticFrames() : noFrames,
                               /* scenarioAcceptsAnyUtterance= */ (*ref)->GetScenario().AcceptsAnyUtterance()};

    scenarioResponse.ForceBuilderFromSession(ctx.SpeechKitRequest(), applyState.Request.GetRef(), GuidGenerator, *session);
    response.AddScenarioResponse(std::move(scenarioResponse));

    return applyState;
}

TWalkerResponse TCommonScenarioWalker::ApplySideEffects(ILightWalkerRequestCtx& walkerCtx) const {
    const auto& ctx = walkerCtx.Ctx();
    TRequestCtx& requestCtx = walkerCtx.RequestCtx();
    auto& logger = requestCtx.RTLogger();

    auto applyStateOrError = RestoreApplyState(walkerCtx);

    if (applyStateOrError.Error()) {
        return TWalkerResponse{*applyStateOrError.Error()};
    }

    auto& applyState = applyStateOrError.Value();
    auto& response = applyState.WalkerResponse;
    if (applyState.ActionEffect.Status == TActionEffect::EStatus::WalkerResponseIsComplete) {
        return std::move(response);
    }
    if (response.Scenarios.empty()) {
        const auto error = TError{TError::EType::Logic} << "No scenario response after Restore Apply State";
        LOG_ERR(ctx.Logger()) << error;
        return TWalkerResponse{error};
    }
    if (!applyState.Request.Defined()) {
        const auto error = TError{TError::EType::Logic} << "No request after Restore Apply State";
        LOG_ERR(ctx.Logger()) << error;
        return TWalkerResponse{error};
    }

    auto& analyticsInfoBuilder = response.AnalyticsInfoBuilder;
    auto& wrapper = applyState.ScenarioWrapper;
    auto& scenarioResponse = response.Scenarios.front();
    const auto& scenarioName = scenarioResponse.GetScenarioName();

    THashSet<TString> scenariosWithTunnellerResponses =
        NImpl::GetScenariosFromFlag(ctx.ExpFlag(EXP_MOVE_TUNNELLER_RESPONSES_FROM_SCENARIOS));

    const auto& proactivity = ctx.Session() ? ctx.Session()->GetProactivityRecommendations() : TProactivityAnswer{};

    const auto postAnalyticsFiller = [&](TScenarioWrapperPtr wrapper) {
        if (wrapper) {
            const auto analyticsInfo = std::move(wrapper->GetAnalyticsInfo()).Build();
            if (scenariosWithTunnellerResponses.contains(NImpl::ALL_SCENARIOS) ||
                scenariosWithTunnellerResponses.contains(scenarioName)) {
                analyticsInfoBuilder.CopyTunnellerRawResponses(scenarioName, analyticsInfo);
            }
            analyticsInfoBuilder.AddAnalyticsInfo(scenarioResponse.GetScenarioName(), analyticsInfo);
        }
    };

    const TErrorOr<EApplyResult> result = TryApplyAndFinalizeOrRenderError(
        false /* disableApply */, scenarioResponse, wrapper, applyState.Request.GetRef(), walkerCtx, logger, ctx.Language(),
        analyticsInfoBuilder, response.ProactivityLogStorage, response.QualityStorage, proactivity, ECalledFrom::ApplyStage,
        GetMatchedSemanticFrames(ctx.Session(), wrapper, ECalledFrom::ApplyStage),
        postAnalyticsFiller);

    if (const EApplyResult* applyResult = result.TryValue()) {
        switch (*applyResult) {
            case EApplyResult::Called:
                // It's okay and expected.
                break;
            case EApplyResult::Deferred:
                // It's unexpected and, generally speaking, is a bug.
                Y_ASSERT(false);
                LOG_ERR(logger) << "Deferred apply during deferred apply call for scenario " << scenarioName;
                break;
        }
    }

    return std::move(response);
}

[[nodiscard]] TErrorOr<EApplyResult>
TCommonScenarioWalker::TryApplyAndFinalize(const bool disableApply, TScenarioResponse& response,
                                           TScenarioWrapperPtr wrapper, const TRequest& request,
                                           ILightWalkerRequestCtx& walkerCtx, TRTLogger& logger,
                                           NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                           NMegamind::TProactivityLogStorage& proactivityLogStorage,
                                           const TQualityStorage& storage,
                                           const TProactivityAnswer& proactivity,
                                           ECalledFrom /* calledFrom */,
                                           const TVector<TSemanticFrame>& matchedSemanticFrames,
                                           const TPostAnalyticsFiller& postAnalyticsFiller) const {
    if (!wrapper) {
        return TError{TError::EType::Logic} << "Wrapper is not set";
    }

    auto& ctx = walkerCtx.Ctx();
    const auto scenarioName = wrapper->GetScenario().GetName();

    EApplyResult result = EApplyResult::Called;
    if (wrapper->SetReasonWhenNonApplicable(request, ctx, response) == EApplicability::Inapplicable) {
        LOG_INFO(logger) << "Scenario " << scenarioName << " is not applicable.";
    } else if (!disableApply) {
        if (wrapper->GetDeferredApplyMode() == EDeferredApplyMode::DeferApply) {
            if (const auto error =
                    FinalizeWrapperAndPostFillAnalytics(wrapper, postAnalyticsFiller, request, ctx, response)) {
                return *error;
            }
        }
        TErrorOr<EApplyResult> status = wrapper->StartApply(
            request, ctx, response, analyticsInfoBuilder.BuildProto(), storage, proactivity);
        if (auto* error = status.Error()) {
            LOG_ERR(logger) << "Scenario " << scenarioName << " start apply error: " << *error;
            return std::move(*error);
        }
        LOG_INFO(logger) << "Scenario " << scenarioName << " start apply success";

        if (status.Value() == EApplyResult::Deferred || walkerCtx.RunStage() == ILightWalkerRequestCtx::ERunStage::ApplyPrepareScenario) {
            return status.Value();
        }
        status = wrapper->FinishApply(request, ctx, response);
        if (auto* error = status.Error()) {
            LOG_ERR(logger) << "Scenario " << scenarioName << " finish apply error: " << *error;
            return std::move(*error);
        }
        LOG_INFO(logger) << "Scenario " << scenarioName << " finish apply success";
        Y_ASSERT(status.IsSuccess());
        result = status.Value();
    }

    if (result == EApplyResult::Called) {
        if (const auto error = walkerCtx.ModifierRequestFactory().ApplyModifierResponse(response, analyticsInfoBuilder)) {
            return *error;
        }
        if (const auto error =
                FinalizeWrapperAndPostFillAnalytics(wrapper, postAnalyticsFiller, request, ctx, response)) {
            return *error;
        }
    }

    if (!response.BuilderIfExists()) {
        return TError{TError::EType::Logic} << "Scenario " << scenarioName << " render error: no builder";
    }
    if (result == EApplyResult::Called) {
        NMegamind::TModifiersInfo modifiersInfo;

        TVector<TSemanticFrame> semanticFramesToMatchPostroll = matchedSemanticFrames;

        const bool postrollFramesFromResponse = semanticFramesToMatchPostroll.empty();
        if (postrollFramesFromResponse) {
            semanticFramesToMatchPostroll.push_back(response.GetResponseSemanticFrame());
        }

        ApplyResponseModifiers(response, walkerCtx, logger, wrapper->GetModifiersStorage(), modifiersInfo,
                               proactivityLogStorage, proactivity, semanticFramesToMatchPostroll,
                               request.GetRecognizedActionEffectFrames(), MakeModifiers(), analyticsInfoBuilder);

        const TString proactivitySource = NMegamind::GetProactivitySource(ctx.Session(), response);
        modifiersInfo.MutableProactivity()->SetSource(proactivitySource);

        auto semanticFramesInfo = modifiersInfo.MutableProactivity()->MutableSemanticFramesInfo();
        semanticFramesInfo->SetSource(
            postrollFramesFromResponse
            ? NMegamind::TProactivityInfo_TFramesToMatchPostrollInfo_EFrameSource_Response
            : NMegamind::TProactivityInfo_TFramesToMatchPostrollInfo_EFrameSource_Begemot);
        for (const auto& frame : semanticFramesToMatchPostroll) {
            *semanticFramesInfo->AddSemanticFrames() = frame;
        }

        analyticsInfoBuilder.SetModifiersInfo(modifiersInfo);
    }
    if (const auto error = Finalize(request, response, wrapper, walkerCtx)) {
        return TError{TError::EType::Logic} << "Scenario " << scenarioName << " finalize error: " << *error;
    }

    return result;
}

[[nodiscard]] bool TCommonScenarioWalker::TryRenderError(const TRequest& requestModel, const TError& error, ELanguage language,
                                                         TScenarioResponse& response,
                                                         ILightWalkerRequestCtx& walkerCtx, TRTLogger& logger) const {
    response.SetHttpCode(HTTP_UNASSIGNED_512, error.ErrorMsg);

    auto* builder = response.BuilderIfExists();
    if (!builder) {
        LOG_WARN(logger) << "Build doesn't exist";
        builder = &response.ForceBuilder(walkerCtx.Ctx().SpeechKitRequest(), requestModel, GuidGenerator);
    } else {
        builder->Reset(walkerCtx.Ctx().SpeechKitRequest(), requestModel, GuidGenerator);
    }

    builder->AddError(ToString(error.Type), TString::Join(response.GetScenarioName(), ": ", error.ErrorMsg));

    try {
        const auto phrase = walkerCtx.GlobalCtx().GetNlgRenderer().RenderPhrase(
            "error", "error", language, walkerCtx.Rng(), NNlg::TRenderContextData());
        builder->AddSimpleText(phrase.Text, phrase.Voice);
        return true;
    } catch (...) {
        const auto errorMessage = CurrentExceptionMessage();
        builder->AddError(ToString(TError::EType::System), "Nlg render error: " + errorMessage);
        LOG_ERR(logger) << "Can not render nlg error: " << errorMessage;
        return false;
    }
}

[[nodiscard]] TErrorOr<EApplyResult> TCommonScenarioWalker::TryApplyAndFinalizeOrRenderError(
    const bool disableApply, TScenarioResponse& response, TScenarioWrapperPtr wrapper,
    const TRequest& request, ILightWalkerRequestCtx& walkerCtx, TRTLogger& logger, ELanguage language,
    NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
    NMegamind::TProactivityLogStorage& proactivityLogStorage,
    const TQualityStorage& storage,
    const TProactivityAnswer& proactivity, ECalledFrom calledFrom,
    const TVector<TSemanticFrame>& matchedSemanticFrames,
    const TPostAnalyticsFiller& postAnalyticsFiller) const {
    const TString scenarioName = response.GetScenarioName();
    TErrorOr<EApplyResult> result = TryApplyAndFinalize(disableApply, response, wrapper, request, walkerCtx, logger,
                                                        analyticsInfoBuilder, proactivityLogStorage, storage, proactivity,
                                                        calledFrom, matchedSemanticFrames, postAnalyticsFiller);

    auto* error = result.Error();
    if (!error) {
        response.SetHttpCode(HTTP_OK);
        return result;
    }

    LOG_ERR(logger) << "Failed to call apply for scenario " << scenarioName << ": " << *error;
    if (!TryRenderError(request, *error, language, response, walkerCtx, logger)) {
        LOG_ERR(logger) << "Failed to render error for scenario " << scenarioName;
        return TError(TError::EType::Logic) << "Failed to render error nlg for scenario " << scenarioName;
    }

    return std::move(*error);
}

TStatus TCommonScenarioWalker::Finalize(const TRequest& request, TScenarioResponse& response,
                                        TScenarioWrapperPtr wrapper,
                                        ILightWalkerRequestCtx& walkerCtx) const {
    return TResponseFinalizer{
        wrapper, walkerCtx, request, response.GetScenarioName(),
        response.GetFeatures(), response.GetRequestIsExpected()}.Finalize(response.BuilderIfExists());
}

void TCommonScenarioWalker::ApplyResponseModifiers(
    TScenarioResponse& response, ILightWalkerRequestCtx& walkerCtx, TRTLogger& logger,
    NMegamind::TModifiersStorage& modifiersStorage, NMegamind::TModifiersInfo& modifiersInfo,
    NMegamind::TProactivityLogStorage& proactivityLogStorage, const TProactivityAnswer& proactivity,
    const TVector<TSemanticFrame>& semanticFramesToMatchPostroll, const TVector<TSemanticFrame>& recognizedActionEffectFrames,
    const TVector<NMegamind::TModifierPtr>& modifiers,
    NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder) const
{
    LOG_DEBUG(logger) << "Applyling response modifiers";
    auto& ctx = walkerCtx.Ctx();
    NMegamind::TResponseModifierContext modCtx(ctx,
                                               modifiersStorage,
                                               modifiersInfo,
                                               proactivityLogStorage,
                                               semanticFramesToMatchPostroll,
                                               recognizedActionEffectFrames,
                                               analyticsInfoBuilder,
                                               proactivity);
    for (const auto& modifier : modifiers) {
        const auto modName = modifier->GetName() + " modifier";
        const auto applyResult = modifier->TryApply(modCtx, response);
        if (const auto* error = applyResult.Error()) {
            LOG_ERROR(logger) << "Exception in " << modName << ": " << *error;
        } else if (const auto nonApply = applyResult.Value()) {
            LOG_INFO(logger) << "Did not apply " << modName << (nonApply->Reason() ? (": " + nonApply->Reason()) : "");
        } else {
            LOG_INFO(logger) << "Applied " << modName;
        }
    }
}

void TCommonScenarioWalker::InitScenario(const TRequest& request, const IContext& ctx, NMegamind::IDataSources& dataSources,
                                         const TScenario& scenario, TScenarioWrapperPtr& wrapper,
                                         TWalkerResponse& response) const {
    Y_ASSERT(wrapper);

    const auto& scenarioName = scenario.GetName();
    if (const auto error = wrapper->Init(request, ctx, dataSources)) {
        LOG_ERR(ctx.Logger()) << "Scenario " << scenarioName << " init error: " << error;
        response.AddScenarioError(scenario.GetName(), NImpl::STAGE_INIT, *error);
    } else {
        LOG_WITH_TYPE(ctx.Logger(), TLOG_INFO, ELogMessageType::MegamindPrepareScenarioRunRequests)
            << "Scenario " << scenarioName << " init success";
    }
}

void TCommonScenarioWalker::AskScenario(const TRequest& request, const IContext& ctx,
                                        const TFactorStorage& /* factorStorage */, const TFormulasStorage& /* formulasStorage */,
                                        const TScenario& scenario, TScenarioWrapperPtr& wrapper,
                                        TWalkerResponse& response) const {
    std::call_once(wrapper->GetAskFlag(), [&]() {
        Y_ASSERT(wrapper);
        if (!wrapper->IsSuccess()) {
            return;
        }

        std::unique_ptr<TScenarioResponse> r = std::make_unique<TScenarioResponse>(
            scenario.GetName(), wrapper->GetSemanticFrames(), scenario.AcceptsAnyUtterance());

        if (const auto error = wrapper->Ask(request, ctx, *r)) {
            LOG_ERR(ctx.Logger()) << "Scenario " << scenario.GetName() << " ask error: " << error;
            response.AddScenarioError(scenario.GetName(), NImpl::STAGE_ASK, *error);
        } else {
            LOG_WITH_TYPE(ctx.Logger(), TLOG_INFO, ELogMessageType::MegamindRetrieveScenarioRunResponses)
                << "Scenario " << scenario.GetName() << " ask success";

            response.AddScenarioResponse(std::move(*r));
        }
    });
}

void TCommonScenarioWalker::ContinueScenario(const TRequest& request, const IContext& ctx, const TScenario& scenario,
                                             TScenarioWrapperPtr& wrapper, TWalkerResponse& response) const {
    Y_ASSERT(wrapper);
    TScenarioResponse r{scenario.GetName(), wrapper->GetSemanticFrames(),
                        /* scenarioAcceptsAnyUtterance= */ scenario.AcceptsAnyUtterance()};

    if (const auto error = wrapper->StartHeavyContinue(request, ctx)) {
        LOG_ERR(ctx.Logger()) << "Scenario " << scenario.GetName() << " continue error: " << error;
        response.AddScenarioError(scenario.GetName(), NImpl::STAGE_CONTINUE, *error);
    }
}

void TCommonScenarioWalker::InitScenarios(const TScenarioWrapperPtrs& wrappers, const TRequest& request,
                                          const IContext& ctx, const TFactorStorage& factorStorage,
                                          const TFormulasStorage& formulasStorage,
                                          NMegamind::IDataSources& dataSources,
                                          ILightWalkerRequestCtx::ERunStage runStage, TWalkerResponse& response,
                                          NMegamind::TItemProxyAdapter& itemAdapter) const {
    TScenarioWrapperPtrs initializedWrappers; // Guarded by lock.

    const auto initFunction = [&](const TScenario& scenario, TScenarioWrapperPtr wrapper) {
        auto requiredDataSources = scenario.GetRequiredDataSources();
        NMegamind::NImpl::AddScenarioDataSourcesByExp(scenario.GetName(), ctx.ExpFlags(), requiredDataSources);
        // FIXME(g-kostin): we temporally allow callbacks to bypass datasource requirements
        bool areRequiredDataSourcesFilled = request.GetEvent().AsServerActionEvent() ||
            AllOf(requiredDataSources, [&dataSources](const EDataSourceType type) {
                return NMegamind::IsDataSourceFilled(dataSources.GetDataSource(type));
            });

        if (areRequiredDataSourcesFilled) {
            InitScenario(request, ctx, dataSources, scenario, wrapper, response);
            initializedWrappers.push_back(wrapper);
            if (itemAdapter.CheckFlagInInputContext(NMegamind::AH_FLAG_EXPECT_WEBSEARCH_RESPONSE)) {
                NImpl::AddSearchRelatedScenarioStats(ctx, wrapper, EHeavyScenarioEvent::Request);
            }
        } else {
            LOG_INFO(ctx.Logger()) << "Not all required DataSources are filled for scenario "
                                    << scenario.GetName() << ". Skipping.";
        }
    };

    OnlyFor<TConfigBasedAppHostProxyProtocolScenario, TConfigBasedAppHostPureProtocolScenario>(wrappers, initFunction);

    NMegamindAppHost::TLaunchedScenarios launchedScenarios;
    for (const auto& wrapper : initializedWrappers) {
        auto& scenario = *launchedScenarios.AddScenarios();
        scenario.SetName(wrapper->GetScenario().GetName());
    }
    LOG_DEBUG(ctx.Logger()) << "Launched scenarios: " << launchedScenarios.ShortUtf8DebugString();
    itemAdapter.PutIntoContext(launchedScenarios, NMegamind::AH_ITEM_LAUNCHED_SCENARIOS);

    if (runStage != ILightWalkerRequestCtx::ERunStage::PreClassification) {
        OnlyFor<TConfigBasedAppHostProxyProtocolScenario,
                TConfigBasedAppHostPureProtocolScenario>(
            initializedWrappers,
            [&, this](const TScenario& scenario, TScenarioWrapperPtr wrapper) {
                AskScenario(request, ctx, factorStorage, formulasStorage, scenario, wrapper, response);
            });
    }
}

void TCommonScenarioWalker::ContinueHeavyScenario(TScenarioWrapperPtr wrapper, const TRequest& request,
                                                   const IContext& ctx, TWalkerResponse& response) const {
    OnlyVisit<TConfigBasedAppHostProxyProtocolScenario>(wrapper, [&](const TScenario& scenario) {
        std::call_once(wrapper->GetContinueFlag(), [&]() mutable {
            if (wrapper->IsSuccess()) {
                ContinueScenario(request, ctx, scenario, wrapper, response);
            }
        });
    });
    OnlyVisit<TConfigBasedAppHostPureProtocolScenario>(wrapper, [&](const TScenario& scenario) {
        std::call_once(wrapper->GetContinueFlag(), [&]() mutable {
            if (wrapper->IsSuccess()) {
                ContinueScenario(request, ctx, scenario, wrapper, response);
            }
        });
    });
}

void TCommonScenarioWalker::PreFillAnalyticsInfo(NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                 const IContext& ctx) const {
    analyticsInfoBuilder.SetChosenUtterance(ctx.PolyglotUtterance());

    auto& event = ctx.SpeechKitRequest().Event();
    if (event.GetType() == EEventType::voice_input && event.HasOriginalZeroAsrHypothesisIndex()) {
        const auto& originalZeroAsrHypothesis = event.GetAsrResult(event.GetOriginalZeroAsrHypothesisIndex());
        TVector<TString> originalZeroAsrHypothesisWords;
        for (const auto& word : originalZeroAsrHypothesis.GetWords()) {
            originalZeroAsrHypothesisWords.push_back(word.GetValue());
        }
        analyticsInfoBuilder.SetOriginalUtterance(JoinSeq(" ", originalZeroAsrHypothesisWords));
    } else {
        analyticsInfoBuilder.SetOriginalUtterance(ctx.PolyglotUtterance());
    }

    if (event.GetType() == EEventType::voice_input && event.HasOriginalZeroAsrHypothesisIndex()) {
        const auto& originalZeroHypo = event.GetAsrResult(event.GetOriginalZeroAsrHypothesisIndex());
        const auto& shownUtterance =
            originalZeroHypo.HasNormalized() ? originalZeroHypo.GetNormalized() : originalZeroHypo.GetUtterance();
        analyticsInfoBuilder.SetShownUtterance(shownUtterance);
    } else {
        if (const auto normalizedUtterance = ctx.AsrNormalizedUtterance()) {
            analyticsInfoBuilder.SetShownUtterance(ToString(*normalizedUtterance));
        } else {
            analyticsInfoBuilder.SetShownUtterance(ctx.PolyglotUtterance());
        }
    }
    analyticsInfoBuilder.SetDeviceStateActions(ctx.SpeechKitRequest()->GetRequest().GetDeviceState().GetActions());
    if (ctx.HasIoTUserInfo()) {
        analyticsInfoBuilder.SetIoTUserInfo(ctx.IoTUserInfo());
    }
    if (ctx.HasResponses() && ctx.Responses().BlackBoxResponse().HasUserInfo()) {
        const auto& userInfo = ctx.Responses().BlackBoxResponse().GetUserInfo();
        NMegamind::TUserProfile profile;
        profile.MutableSubscriptions()->CopyFrom(userInfo.GetSubscriptions());
        profile.SetHasYandexPlus(userInfo.GetHasYandexPlus());
        analyticsInfoBuilder.SetUserProfile(profile);
    }
}

void TCommonScenarioWalker::PostFillAnalyticsInfo(NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                  const IContext& ctx, const TScenarioWrapperPtrs& scenarios,
                                                  TScenarioWrapperPtr winnerScenario,
                                                  const THashSet<TString>& scenariosWithTunnellerResponses,
                                                  const TRequest& requestModel) const {
    if (scenariosWithTunnellerResponses.contains(NImpl::ALL_SCENARIOS) ||
        scenariosWithTunnellerResponses.contains(NImpl::MEGAMIND_TUNNELLER_RESPONSE_KEY)) {
        TStatus error;
        const auto& webSearchResponse = ctx.Responses().WebSearchResponse(&error);
        if (!error) {
            if (const auto tunnellerRawResponse = webSearchResponse.GetTunnellerRawResponse(); tunnellerRawResponse.Defined()) {
                analyticsInfoBuilder.AddTunnellerRawResponse(NImpl::MEGAMIND_TUNNELLER_RESPONSE_KEY, *tunnellerRawResponse);
            }
        }
    }

    for (const auto& scenario : scenarios) {
        const auto& scenarioName = scenario->GetScenario().GetName();
        auto& scenarioAnalyticsInfoBuilder = scenario->GetAnalyticsInfo();
        const bool isWinnerScenario = scenario == winnerScenario;

        if (isWinnerScenario) {
            scenarioAnalyticsInfoBuilder.SetMatchedSemanticFrames(winnerScenario->GetSemanticFrames());
        }

        const auto analyticsInfo = std::move(scenarioAnalyticsInfoBuilder).Build();
        if (scenariosWithTunnellerResponses.contains(NImpl::ALL_SCENARIOS) ||
            scenariosWithTunnellerResponses.contains(scenarioName))
        {
            analyticsInfoBuilder.CopyTunnellerRawResponses(scenarioName, analyticsInfo);
        }
        if (isWinnerScenario) {
            analyticsInfoBuilder.AddAnalyticsInfo(scenarioName, analyticsInfo);
            analyticsInfoBuilder.SetWinnerScenarioName(scenarioName);
        }

        if (const auto userInfo = std::move(scenario->GetUserInfo()).Build()) {
            analyticsInfoBuilder.AddUserInfo(scenarioName, *userInfo);
        }

        analyticsInfoBuilder.AddScenarioTimings(scenarioName, analyticsInfo);
    }
    if (const auto& location = requestModel.GetLocation(); location.Defined()) {
        analyticsInfoBuilder.SetLocation(location.GetRef());
    }
}

TVector<TSemanticFrame> TCommonScenarioWalker::GetMatchedSemanticFrames(const ISession* session, const TScenarioWrapperPtr& wrapper,
                                                                        ECalledFrom calledFrom) const {
    if (calledFrom == ECalledFrom::RunStage) {
        return wrapper->GetSemanticFrames();
    }
    TVector<TSemanticFrame> matchedSemanticFrames;
    if (const auto input = session ? session->GetInput() : Nothing()) {
        for (const auto& frame : input->GetSemanticFrames()) {
            matchedSemanticFrames.push_back(frame);
        }
    }
    return matchedSemanticFrames;
}

const NMegamind::IGuidGenerator& TCommonScenarioWalker::GetGuidGenerator() const {
    return GuidGenerator;
}

void TCommonScenarioWalker::SavePostClassifyState(IRunWalkerRequestCtx& walkerCtx, IContext& ctx,
                                                  NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                  const TScenarioWrapperPtrs& scenarioWrappers,
                                                  THashSet<TString>& scenariosWithTunnellerResponses,
                                                  const TRequest& request,
                                                  const TWalkerResponse& response,
                                                  const TStatus& postClassifyError,
                                                  TScenarioWrapperPtr wrapper) const
{
    LOG_DEBUG(ctx.Logger()) << "End of PostClassification saving items to apphost context";

    PostFillAnalyticsInfo(analyticsInfoBuilder, ctx, scenarioWrappers, /* winnerScenario= */ nullptr,
                            scenariosWithTunnellerResponses, request);

    walkerCtx.SavePostClassifyState(response, analyticsInfoBuilder, postClassifyError, wrapper, request);
    return;
}

} // namespace NAlice
