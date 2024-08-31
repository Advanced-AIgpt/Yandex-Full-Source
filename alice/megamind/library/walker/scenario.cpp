#include "scenario.h"
#include "response.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/misspell/misspell.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_wrapper.h>

#include <util/generic/string.h>
#include <util/string/builder.h>

namespace NAlice {

namespace {

bool TryGetRequestModelFromContext(NMegamind::TItemProxyAdapter& itemProxyAdapter,
                                   TMaybe<IScenarioWalker::TRequestState>& requestState,
                                   IScenarioWalker::TRunState& runState)
{
    const auto errorOrRequest =
        itemProxyAdapter.GetFromContext<NMegamind::TRequestData>(NMegamind::AH_ITEM_MM_REQUEST_DATA);
    auto errorOrAnalytics =
        itemProxyAdapter.GetFromContext<NMegamind::TMegamindAnalyticsInfo>(
            NMegamind::AH_ITEM_MM_RUN_STATE_ANALYTICS_WALKER_PREPARE);
    if (errorOrRequest.Error() || errorOrAnalytics.Error()) {
        return false;
    }

    requestState = IScenarioWalker::TRequestState{CreateRequest(errorOrRequest.Value())};
    runState.AnalyticsInfoBuilder.CopyFromProto(std::move(errorOrAnalytics.Value()));
    return true;
}

} // namespace

TStatus IScenarioWalker::RunPrepare(IRunWalkerRequestCtx& walkerCtx, TRunState& runState,
                                    TMaybe<TRequestState>& requestState,
                                    TMaybe<TPreClassifyState>& preClassifyState) const
{
    const auto& ctx = walkerCtx.Ctx();
    auto& requestCtx = walkerCtx.RequestCtx();

    const TString timerStageSuffix = TStringBuilder{} << '.' << walkerCtx.RunStage();

    requestCtx.StageTimers().RegisterAndSignal(requestCtx,
                                               TString::Join(NMegamind::TS_STAGE_WALKER_BEFORE_PREPARE_REQUEST, timerStageSuffix),
                                               NMegamind::TS_STAGE_START_REQUEST,
                                               walkerCtx.Ctx().Sensors());

    if (!TryGetRequestModelFromContext(walkerCtx.ItemProxyAdapter(), requestState, runState)) {
        requestState = RunPrepareRequest(walkerCtx, runState);
    }

    requestCtx.StageTimers().RegisterAndSignal(requestCtx,
                                               TString::Join(NMegamind::TS_STAGE_WALKER_AFTER_PREPARE_REQUEST, timerStageSuffix),
                                               NMegamind::TS_STAGE_START_REQUEST,
                                               walkerCtx.Ctx().Sensors());
    if (!requestState.Defined()) {
        return TError{TError::EType::Critical} << "unable to create requestState";
    }

    requestCtx.StageTimers().RegisterAndSignal(requestCtx,
                                               TString::Join(NMegamind::TS_STAGE_WALKER_BEFORE_PRECLASSIFY, timerStageSuffix),
                                               NMegamind::TS_STAGE_START_REQUEST,
                                               walkerCtx.Ctx().Sensors());
    if (auto err = RunPreClassify(walkerCtx, runState, *requestState, preClassifyState)) {
        return std::move(*err);
    }
    requestCtx.StageTimers().RegisterAndSignal(requestCtx,
                                               TString::Join(NMegamind::TS_STAGE_WALKER_AFTER_PRECLASSIFY, timerStageSuffix),
                                               NMegamind::TS_STAGE_START_REQUEST,
                                               walkerCtx.Ctx().Sensors());

    if (preClassifyState->IsTrashPartial) {
        MakeTrashPartialResponse(requestState->Request, runState.Response, ctx);
        return TError{TError::EType::Input} << "It is a trash partial";
    }

    return Success();
}

void IScenarioWalker::MakeTrashPartialResponse(const TRequest& request, TWalkerResponse& response,
                                               const IContext& ctx) const
{
    TScenarioResponse scenarioResponse(/* scenarioName= */ {}, /* scenarioSemanticFrames= */ {},
                                       /* scenarioAcceptsAnyUtterance= */ false);
    auto& builder = scenarioResponse.ForceBuilder(ctx.SpeechKitRequest(), request, GetGuidGenerator());
    builder.SetSession(/* dialogId= */ request.GetDialogId().GetOrElse(""), ctx.SpeechKitRequest()->GetSession())
           .SetIsTrashPartial(true);
    response.AddScenarioResponse(std::move(scenarioResponse));
    response.ApplyResult = EApplyResult::Called;
}

} // namespace NAlice
