#pragma once

#include "request_frame_to_scenario_matcher.h"
#include "requestctx.h"
#include "response.h"
#include "scenario.h"
#include "talkien.h"

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/modifiers/modifier.h>
#include <alice/megamind/library/proactivity/common/common.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_wrapper.h>
#include <alice/megamind/library/scenarios/helpers/scenario_wrapper.h>
#include <alice/megamind/library/scenarios/interface/data_sources.h>
#include <alice/megamind/library/scenarios/registry/registry.h>
#include <alice/megamind/library/stage_wrappers/postclassify_state.h>
#include <alice/megamind/protos/common/effect_options.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/names.h>
#include <alice/library/typed_frame/typed_semantic_frame_request.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <functional>
#include <memory>

namespace NAlice {

namespace NTestSuiteWalker {
struct TTestCaseBuilderTryRenderNlg;
struct TTestCaseTestOnNoWinnerScenarioResponse;
struct TTestCaseTestOnNoWinnerScenarioResponseWithoutResponseBuilder;
struct TTestCaseApplyResponseModifiers;
struct TTestCaseAsyncScenarioAsk;
struct TTestCaseTCommonScenarioWalker_RegisterScenarios;
struct TTestCaseFillAnalyticsInfoWithSelectedWrapper;
struct TTestCaseFillAnalyticsInfoWithSelectedWrapperAndEmptyAnalyticsInfo;
struct TTestCaseFillAnalyticsInfoWithSelectedWrapperAndOnlyTunnellerAnalyticsInfo;
struct TTestCaseFillAnalyticsInfoWithoutSelectedWrapperAndEmptyAnalyticsInfo;
struct TTestCaseFillAnalyticsInfoWithoutSelectedWrapper;
struct TTestCaseFillAnalyticsInfoWithScenarioTimings;
struct TTestCaseFillAnalyticsInfoWithParentProductScenarioName;
struct TTestCaseFillAnalyticsWithLocation;
struct TTestCaseParallelScenarioInit;
struct TTestCaseTCommonScenarioWalker_Run;
struct TTestCaseTryApplyAndFinalizeOrRenderErrorHttpCodeNotSet;
struct TTestCaseVersionInResponse;
struct TTestCaseSkipScenariosWithEmptyRequiredDataSource;
struct TTestCaseDoNotSkipScenarioWithEmptyRequiredDataSourceOnCallback;
struct TTestCasePassResponseFrameToProactivity;
struct TTestCasePassResponseFrameToProactivityWithMemento;
struct TTestCasePassResponseFrameToProactivityVins;
struct TTestCasePassBegemotFramesToProactivity;
struct TTestCasePassBegemotFramesToProactivityWithMemento;
struct TTestCaseTestGetMatchedSemanticFrames;
struct TTestCaseTestSendUdpMetrics;
struct TTestCaseFinalizeWrapper;
struct TTestCaseAskModifiersSuccessWalkerRun;
struct TTestCaseAskModifiersFailure;
}  // namespace NTestSuiteWalker

namespace NTestImpl {
std::variant<TRequest, TWalkerResponse> TestPreProcessRequest(const TSpeechKitRequest& speechKitRequest,
                                                          const NMegamind::TStackEngineCore& stackEngineCore);
} // NTestImpl

namespace NTestSuiteFramesToScenarioMapping {
    struct TTestCaseAdditionalFramesSubscriptionByExperiment;
} // NTestSuiteFramesToScenarioMapping

class TCommonScenarioWalker : public IScenarioWalker {
public:

    enum ECalledFrom {
        RunStage,
        ApplyStage
    };

    enum class EHeavyScenarioEvent {
        Request /* "request" */,
        Win     /* "win" */
    };

    class IEarlyContinueClassifier {
    public:
        virtual ~IEarlyContinueClassifier() = default;
        virtual bool IsScenarioAllowed(TStringBuf scenarioName) = 0;
    };

public:
    explicit TCommonScenarioWalker(IGlobalCtx& globalCtx);

    // Overrides of IScenarioWalker.
    TErrorOr<TApplyState> RestoreApplyState(ILightWalkerRequestCtx& walkerCtx) const override;

    TWalkerResponse RunPostClassifyStage(IRunWalkerRequestCtx& walkerCtx) const override;
    TWalkerResponse RunPreClassifyStage(IRunWalkerRequestCtx& walkerCtx) const override;
    TWalkerResponse RunProcessContinueStage(IRunWalkerRequestCtx& walkerCtx) const override;
    TWalkerResponse RunFinalizeStage(IRunWalkerRequestCtx& walkerCtx) const override;

    TWalkerResponse ApplySideEffects(ILightWalkerRequestCtx& walkerCtx) const override;
    TStatus ModifyApplyScenarioResponse(ILightWalkerRequestCtx& walkerCtx,
                                        TErrorOr<TApplyState>&& applyState) const override;

    TWalkerResponse RunFinalize(IRunWalkerRequestCtx& walkerCtx, TPreClassifyState& preClassifyState,
                                const TRequest& request,
                                NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsFromPrepare) const override;
    TScenarioWrapperPtr RunStartContinue(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                     TPreClassifyState& preClassifyState) const override;
    void RunFinishContinue(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                     TPreClassifyState& preClassifyState) const override;
    void RunClassifyWinner(IRunWalkerRequestCtx& walkerCtx, const TRequest& requestModel) const override;
    void RunProcessCombinatorContinue(IRunWalkerRequestCtx& walkerCtx, const TRequest& requestModel) const override;

    TWalkerResponse RunFinalizeCombinator(IRunWalkerRequestCtx& walkerCtx, TPreClassifyState& preClassifyState,
                                          const TRequest& request, const TCombinatorConfig& combinatorConfig,
                                          NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsInfoBuilder,
                                          NMegamindAppHost::TCombinatorProto::ECombinatorStage stage) const;
    TWalkerResponse RunFinalizeScenario(IRunWalkerRequestCtx& walkerCtx, TPreClassifyState& preClassifyState,
                                        const TRequest& request, NMegamind::IPostClassifyState& postClassifyState,
                                        NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsFromPrepare) const;

    TMaybe<TRequestState> RunPrepareRequest(IRunWalkerRequestCtx& walkerCtx, TRunState& runState) const override;
    TStatus RunPreClassify(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, TRequestState& reqState, TMaybe<TPreClassifyState>& out) const override;
    void RunScenarios(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, TRequestState& reqState,
                                 TPreClassifyState& preClassifyState) const override;
    void RunPostClassify(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState, TPreClassifyState& preClassifyState) const override;
    TMaybe<NScenarios::TCombinatorResponse> TryChooseCombinator(IRunWalkerRequestCtx& walkerCtx,
                                                                const TCombinatorConfig& combinatorConfig,
                                                                NMegamindAppHost::TCombinatorProto::ECombinatorStage stage) const;
    TMaybe<NMegamind::TCombinatorResponse> CheckCombinatorResponse(IRunWalkerRequestCtx& walkerCtx) const;
    TStatus RenderCombinatorResponse(IRunWalkerRequestCtx& walkerCtx, TWalkerResponse& walkerResponse,
                                     NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsInfoBuilder,
                                     const TRequest& request, TPreClassifyState& preClassifyState,
                                     const NMegamind::TCombinatorResponse& combinatorResponse) const;

    void OnNoWinnerScenarioResponse(const TRequest& request, TWalkerResponse& response,
                                    const TScenarioWrapperPtrs& scenarioWrappers,
                                    NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsInfoBuilder,
                                    ILightWalkerRequestCtx& walkerCtx, const THashSet<TString>& scenariosWithTunnellerResponses,
                                    const TError& error) const;

    const TScenarioRegistry& GetScenarioRegistry() const {
        return ScenarioRegistry;
    }

    TStatus TryAppHostPreClassify(IRunWalkerRequestCtx& walkerCtx, TScenarioToRequestFrames& scneariosToRequestFrames, TPreClassifyState& preClassifyState) const;
    TErrorOr<TScenarioWrapperPtr> RestorePostClassifyResults(NMegamind::IPostClassifyState& postClassifyState,
                                                             TWalkerResponse& walkerResponse,
                                                             TScenarioWrapperPtrs wrappers, IContext& ctx,
                                                             const TRequest& request) const;
    TErrorOr<TScenarioWrapperPtr>
    RestorePostClassifyAndContinueResults(NMegamind::IPostClassifyState& postClassifyState,
                                          TWalkerResponse& walkerResponse, TScenarioWrapperPtrs wrappers,
                                          IContext& ctx, const TRequest& request,
                                          NMegamind::TItemProxyAdapter& itemAdapter) const;

private:
    // For testing framework.
    friend struct NAlice::NTestSuiteWalker::TTestCaseBuilderTryRenderNlg;
    friend struct NAlice::NTestSuiteWalker::TTestCaseTestOnNoWinnerScenarioResponse;
    friend struct NAlice::NTestSuiteWalker::TTestCaseTestOnNoWinnerScenarioResponseWithoutResponseBuilder;
    friend struct NAlice::NTestSuiteWalker::TTestCaseApplyResponseModifiers;
    friend struct NAlice::NTestSuiteWalker::TTestCaseAsyncScenarioAsk;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsInfoWithSelectedWrapper;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsInfoWithSelectedWrapperAndEmptyAnalyticsInfo;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsInfoWithSelectedWrapperAndOnlyTunnellerAnalyticsInfo;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsInfoWithoutSelectedWrapperAndEmptyAnalyticsInfo;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsInfoWithoutSelectedWrapper;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsInfoWithScenarioTimings;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsInfoWithParentProductScenarioName;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFillAnalyticsWithLocation;
    friend struct NAlice::NTestSuiteWalker::TTestCaseParallelScenarioInit;
    friend struct NAlice::NTestSuiteWalker::TTestCaseTCommonScenarioWalker_RegisterScenarios;
    friend struct NAlice::NTestSuiteWalker::TTestCaseTCommonScenarioWalker_Run;
    friend struct NAlice::NTestSuiteWalker::TTestCaseTryApplyAndFinalizeOrRenderErrorHttpCodeNotSet;
    friend struct NAlice::NTestSuiteWalker::TTestCaseVersionInResponse;
    friend struct NAlice::NTestSuiteWalker::TTestCaseSkipScenariosWithEmptyRequiredDataSource;
    friend struct NAlice::NTestSuiteWalker::TTestCaseDoNotSkipScenarioWithEmptyRequiredDataSourceOnCallback;
    friend std::variant<TRequest, TWalkerResponse>
    NAlice::NTestImpl::TestPreProcessRequest(const TSpeechKitRequest& speechKitRequest,
                                             const NMegamind::TStackEngineCore& stackEngineCore);
    friend struct NAlice::NTestSuiteWalker::TTestCasePassResponseFrameToProactivity;
    friend struct NAlice::NTestSuiteWalker::TTestCasePassResponseFrameToProactivityWithMemento;
    friend struct NAlice::NTestSuiteWalker::TTestCasePassBegemotFramesToProactivity;
    friend struct NAlice::NTestSuiteWalker::TTestCasePassBegemotFramesToProactivityWithMemento;
    friend struct NAlice::NTestSuiteWalker::TTestCasePassResponseFrameToProactivityVins;
    friend struct NAlice::NTestSuiteWalker::TTestCaseTestGetMatchedSemanticFrames;
    friend struct NAlice::NTestSuiteWalker::TTestCaseTestSendUdpMetrics;
    friend struct NAlice::NTestSuiteWalker::TTestCaseFinalizeWrapper;
    friend struct NAlice::NTestSuiteWalker::TTestCaseAskModifiersSuccessWalkerRun;
    friend struct NAlice::NTestSuiteWalker::TTestCaseAskModifiersFailure;

private:
    [[nodiscard]] TErrorOr<EApplyResult>
    TryApplyAndFinalize(const bool disableApply, TScenarioResponse& response, TScenarioWrapperPtr wrapper,
                        const TRequest& request, ILightWalkerRequestCtx& walkerCtx, TRTLogger& logger,
                        NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                        NMegamind::TProactivityLogStorage& proactivityLogStorage, const TQualityStorage& storage,
                        const TProactivityAnswer& proactivity, ECalledFrom calledFrom,
                        const TVector<TSemanticFrame>& matchedSemanticFrames,
                        const TPostAnalyticsFiller& postAnalyticsFiller) const;

    [[nodiscard]] bool TryRenderError(const TRequest& requestModel, const TError& error, ELanguage language,
                                      TScenarioResponse& response, ILightWalkerRequestCtx& walkerCtx, TRTLogger& logger) const;

    [[nodiscard]] TErrorOr<EApplyResult>
    TryApplyAndFinalizeOrRenderError(const bool disableApply, TScenarioResponse& response, TScenarioWrapperPtr wrapper,
                                     const TRequest& request, ILightWalkerRequestCtx& walkerCtx, TRTLogger& logger, ELanguage language,
                                     NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                     NMegamind::TProactivityLogStorage& proactivityLogStorage,
                                     const TQualityStorage& storage, const TProactivityAnswer& proactivity,
                                     ECalledFrom calledFrom, const TVector<TSemanticFrame>& matchedSemanticFrames,
                                     const TPostAnalyticsFiller& postAnalyticsFiller) const;

    TStatus Finalize(const TRequest& request, TScenarioResponse& response, TScenarioWrapperPtr wrapper,
                     ILightWalkerRequestCtx& walkerCtx) const;

    void ApplyResponseModifiers(TScenarioResponse& response, ILightWalkerRequestCtx& walkerCtx,
                                TRTLogger& logger, NMegamind::TModifiersStorage& modifiersStorage,
                                NMegamind::TModifiersInfo& modifiersInfo,
                                NMegamind::TProactivityLogStorage& proactivityLogStorage,
                                const TProactivityAnswer& proactivity,
                                const TVector<TSemanticFrame>& semanticFramesToMatchPostroll,
                                const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                                const TVector<NMegamind::TModifierPtr>& modifiers,
                                NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder) const;

    void MakeFakeScenarioResponse(const TRequest& request, TWalkerResponse& response, const IContext& ctx) const;

    bool ProcessCallbackEvent(const TServerActionEvent& callback,
                              const IContext& ctx,
                              IFrameRequestProcessor& frameRequestProcessor,
                              const TMaybe<TIoTUserInfo>& iotUserInfo,
                              TRunState& runState,
                              NMegamind::TStackEngine& stackEngine,
                              std::unique_ptr<const IEvent>& event,
                              TEffectOptions& effectOptions,
                              TMaybe<TString>& callbackOwnerScenario,
                              NScenarios::TScenarioBaseRequest::ERequestSourceType& requestSource,
                              NMetrics::ISensors& sensors,
                              TRTLogger& logger) const;

    void PreFillAnalyticsInfo(NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                              const IContext& ctx) const;
    void PostFillAnalyticsInfo(NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder, const IContext& ctx,
                               const TScenarioWrapperPtrs& wrappers, TScenarioWrapperPtr wrapper,
                               const THashSet<TString>& scenariosWithTunnellerResponses,
                               const TRequest& requestModel) const;

    void InitScenario(const TRequest& request, const IContext& ctx, NMegamind::IDataSources& dataSources, const TScenario& scenario,
                      TScenarioWrapperPtr& wrapper, TWalkerResponse& response) const;

    void AskScenario(const TRequest& request, const IContext& ctx, const TFactorStorage& factorStorage,
                     const TFormulasStorage& formulasStorage, const TScenario& scenario, TScenarioWrapperPtr& wrapper,
                     TWalkerResponse& response) const;

    void ContinueScenario(const TRequest& request, const IContext& ctx, const TScenario& scenario, TScenarioWrapperPtr& wrapper,
                          TWalkerResponse& response) const;

    void InitScenarios(const TScenarioWrapperPtrs& wrappers,
                       const TRequest& request,
                       const IContext& ctx,
                       const TFactorStorage& factorStorage,
                       const TFormulasStorage& formulasStorage,
                       NMegamind::IDataSources& dataSources,
                       ILightWalkerRequestCtx::ERunStage runStage,
                       TWalkerResponse& response,
                       NMegamind::TItemProxyAdapter& itemAdapter) const;

    TStatus AskScenarios(TScenarioWrapperPtrs& wrappers, const TRequest& request, const IContext& ctx,
                         const TFactorStorage& factorStorage, const TFormulasStorage& formulasStorage,
                         TWalkerResponse& response, NMegamind::TItemProxyAdapter& itemProxyAdapter) const;

    void ContinueHeavyScenario(TScenarioWrapperPtr wrapper, const TRequest& request, const IContext& ctx,
                               TWalkerResponse& response) const;

    TVector<TSemanticFrame> GetMatchedSemanticFrames(const ISession* session, const TScenarioWrapperPtr& wrapper,
                                                     ECalledFrom calledFrom) const;

    void SendUdpSensors(const TVector<NAlice::TScenarioResponse>& responses, NMetrics::ISensors& sensors, bool isWinnerScenario) const;

    TScenarioToRequestFrames ScenarioToRequestFrames(ILightWalkerRequestCtx& walkerCtx, const TRequest& request) const;

    void SavePostClassifyState(IRunWalkerRequestCtx& walkerCtx, IContext& ctx,
                               NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                               const TScenarioWrapperPtrs& scenarioWrappers,
                               THashSet<TString>& scenariosWithTunnellerResponses,
                               const TRequest& request,
                               const TWalkerResponse& resposne,
                               const TStatus& postClassifyError,
                               TScenarioWrapperPtr wrapper) const;
private:
    friend struct NAlice::NTestSuiteFramesToScenarioMapping::TTestCaseAdditionalFramesSubscriptionByExperiment;

protected:
    const NMegamind::IGuidGenerator& GetGuidGenerator() const override;

private:
    TScenarioRegistry ScenarioRegistry;
    NMegamind::TGuidGenerator GuidGenerator;
};

namespace NImpl {

inline const TString MEGAMIND_TUNNELLER_RESPONSE_KEY = "megamind";
inline const TString ALL_SCENARIOS = "ALL_SCENARIOS";
inline constexpr TStringBuf STAGE_ASK = "ask";
inline constexpr TStringBuf STAGE_INIT = "init";
inline constexpr TStringBuf STAGE_CONTINUE = "continue";

TErrorOr<TScenarioWrapperPtrs> RestoreInitializedWrappers(NMegamind::TItemProxyAdapter& itemAdapter,
                                                          const TScenarioWrapperPtrs& wrappers);

void SortWrappers(TScenarioWrapperPtrs& wrappers, const IContext::TExpFlags& expFlags);

bool RaiseErrorOnFailedScenarios(const TScenariosErrors& errors, const TVector<TScenarioResponse>& scenarios,
                                 TStringBuf scenarioNames, TRTLogger& logger);

THashSet<TString> GetScenariosFromFlag(const TMaybe<TString>& scenarios);

void AddSearchRelatedScenarioStats(const IContext& ctx, TScenarioWrapperPtr wrapper,
                                   TCommonScenarioWalker::EHeavyScenarioEvent eventKind);

bool ShouldMakeSearchRequest(const TScenarioToRequestFrames& scenarioToRequestFrames,
                             const IContext& ctx);

void PushFlagsForConditionalDatasources(NMegamind::TItemProxyAdapter& itemAdapter, const IContext& ctx,
                                        TScenarioWrapperPtrs scenarioWrappers);

void CopyScenarioErrorsToWinnerResponse(const TWalkerResponse& walkerResponse, TScenarioResponse& topResponse,
                                        const IContext& ctx);

TScenarioWrapperPtrs MakeScenarioWrappers(const TScenarioToRequestFrames& scenarioToRequestFrames,
                                          const IContext& ctx,
                                          const NMegamind::IGuidGenerator& guidGenerator,
                                          EDeferredApplyMode deferApplyMode,
                                          NMegamind::TItemProxyAdapter& itemProxyAdapter);

bool HasCriticalScenarioVersionMismatch(const TScenariosErrors& errors, TRTLogger& logger);

bool ProcessActionEffect(IScenarioWalker::TActionEffect& actionEffect, IScenarioWalker::TRunState& runState,
                         IFrameRequestProcessor& frameRequestProcessor,
                         bool& utteranceWasUpdated, std::unique_ptr<const IEvent>& event,
                         TVector<TSemanticFrame>& recognizedActionEffectFrames, TVector<TSemanticFrame>& forcedSemanticFrames,
                         TRTLogger& logger, bool doNotForceActionEffectFrameWhenNoUtteranceUpdate);

TErrorOr<TScenarioWrapperPtr> RestoreWinner(TWalkerResponse& walkerResponse, const TString& winnerName,
                                            TScenarioWrapperPtrs wrappers, IContext& ctx, const TRequest& request,
                                            TMaybe<NScenarios::TScenarioContinueResponse> continueResponse);

class TFrameRequestProcessor : public IFrameRequestProcessor {
public:
    TFrameRequestProcessor(
        TVector<TSemanticFrame>& recognizedActionEffectFrames,
        TVector<TSemanticFrame>& forcedSemanticFrames,
        std::unique_ptr<const IEvent>& event,
        TRTLogger& logger,
        std::function<void(const TString&)> processParentProductScenarioName
    )
        : RecognizedActionEffectFrames(recognizedActionEffectFrames)
        , ForcedSemanticFrames(forcedSemanticFrames)
        , Event(event)
        , Logger(logger)
        , ProcessParentProductScenarioName(processParentProductScenarioName)
    { }

    void Process(const TTypedSemanticFrameRequest& frameRequest);

    const TMaybe<TOrigin>& GetOrigin() const { return Origin; }
    const TMaybe<bool>& GetDisableVoiceSession() const { return DisableVoiceSession; }
    const TMaybe<bool>& GetDisableShouldListen() const { return DisableShouldListen; }

private:
    std::reference_wrapper<TVector<TSemanticFrame>> RecognizedActionEffectFrames;
    std::reference_wrapper<TVector<TSemanticFrame>> ForcedSemanticFrames;
    std::reference_wrapper<std::unique_ptr<const IEvent>> Event;
    std::reference_wrapper<TRTLogger> Logger;
    std::function<void(const TString&)> ProcessParentProductScenarioName;
    TMaybe<TOrigin> Origin;
    TMaybe<bool> DisableVoiceSession;
    TMaybe<bool> DisableShouldListen;
};

} // namespace NImpl

} // namespace NAlice
