#pragma once

#include "requestctx.h"
#include "response.h"

#include <alice/megamind/library/classifiers/pre.h>
#include <alice/megamind/library/scenarios/helpers/scenario_wrapper.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/proactivity/proactivity.pb.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>

namespace NAlice {

class IScenarioWalker : public TThrRefBase {
public:
    struct TActionEffect {
        enum EStatus { WalkerResponseIsComplete, WalkerResponseIsNotComplete };

        EStatus Status = WalkerResponseIsComplete;
        std::unique_ptr<const IEvent> UpdatedEvent;
        TMaybe<TSemanticFrame> EffectFrame;
        TVector<TSemanticFrameRequestData> ActionFrames;
    };

    struct TApplyState {
        TScenarioWrapperPtr ScenarioWrapper;
        TWalkerResponse WalkerResponse;
        TActionEffect ActionEffect;
        TMaybe<TRequest> Request;
    };

    virtual TErrorOr<TApplyState> RestoreApplyState(ILightWalkerRequestCtx& walkerCtx) const = 0;

    virtual TWalkerResponse RunPostClassifyStage(IRunWalkerRequestCtx& walkerCtx) const = 0;
    virtual TWalkerResponse RunPreClassifyStage(IRunWalkerRequestCtx& walkerCtx) const = 0;
    virtual TWalkerResponse RunProcessContinueStage(IRunWalkerRequestCtx& walkerCtx) const = 0;
    virtual TWalkerResponse RunFinalizeStage(IRunWalkerRequestCtx& walkerCtx) const = 0;

    virtual TWalkerResponse ApplySideEffects(ILightWalkerRequestCtx& walkerCtx) const = 0;
    virtual TStatus ModifyApplyScenarioResponse(ILightWalkerRequestCtx& walkerctx,
                                                TErrorOr<TApplyState>&& applyState) const = 0;

    struct TRunState {
        NMegamind::TMegamindAnalyticsInfoBuilder AnalyticsInfoBuilder;
        TWalkerResponse Response;
    };

    struct TRequestState {
        TRequest Request;
    };

    struct TPreClassifyState {
        bool DisableApply;
        bool IsTrashPartial = false;
        EDeferredApplyMode DeferredApplyMode;
        TQualityStorage QualityStorage;
        TScenarioWrapperPtrs ScenarioWrappers;
    };

    virtual TWalkerResponse RunFinalize(IRunWalkerRequestCtx& walkerCtx, TPreClassifyState& preClassifyState,
                                        const TRequest& request,
                                        NMegamind::TMegamindAnalyticsInfoBuilder&& analyticsFromPrepare) const = 0;

    virtual TScenarioWrapperPtr RunStartContinue(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                     TPreClassifyState& preClassifyState) const = 0;
    virtual void RunFinishContinue(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                     TPreClassifyState& preClassifyState) const = 0;

    virtual TMaybe<TRequestState> RunPrepareRequest(IRunWalkerRequestCtx& walkerCtx, TRunState& runState) const = 0;
    virtual TStatus RunPreClassify(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, TRequestState& reqState, TMaybe<TPreClassifyState>& out) const = 0;
    virtual void RunScenarios(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, TRequestState& reqState,
                                         TPreClassifyState& preClassifyState) const = 0;
    virtual void RunPostClassify(IRunWalkerRequestCtx& walkerCtx, TRunState& runState, const TRequestState& reqState,
                                 TPreClassifyState& preClassifyState) const = 0;
    virtual void RunClassifyWinner(IRunWalkerRequestCtx& walkerCtx, const TRequest& requestModel) const = 0;
    virtual void RunProcessCombinatorContinue(IRunWalkerRequestCtx& walkerCtx, const TRequest& requestModel) const = 0;

public:
    // Combines RunPrepareRequest and RunPreClassify.
    TStatus RunPrepare(IRunWalkerRequestCtx& wCtx, TRunState& runState,
                       TMaybe<TRequestState>& requestState,
                       TMaybe<TPreClassifyState>& preClassifyState) const;

protected:
    virtual const NMegamind::IGuidGenerator& GetGuidGenerator() const = 0;

    // Helpers.
    void MakeTrashPartialResponse(const TRequest& request, TWalkerResponse& response, const IContext& ctx) const;
};

using TWalkerPtr = TIntrusivePtr<IScenarioWalker>;

} // namespace NAlice
