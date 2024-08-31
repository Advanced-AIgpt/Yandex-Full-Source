#include "walker.h"

#include "walker_apply.h"
#include "walker_monitoring.h"
#include "walker_prepare.h"
#include "walker_run.h"

#include <alice/megamind/library/walker/requestctx.h>
#include <alice/megamind/library/walker/scenario.h>

namespace NAlice::NMegamind {

void RegisterAppHostWalkerHandlers(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TWalkerPtr walker = MakeIntrusive<TCommonScenarioWalker>(globalCtx);

    static const TAppHostPreClassifyNodeHandler preClassifyHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::PreClassification, /* useAppHostStreaming= */ false};
    static const TAppHostPostClassifyNodeHandler walkerPostClassifyHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::PostClassification, /* useAppHostStreaming= */ false};
    static const TAppHostWalkerRunProcessContinueHandler walkerProcessContinueHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::ProcessContinue, /* useAppHostStreaming= */ false};
    static const TAppHostWalkerRunFinalizeNodeHandler walkerRunFinalizeHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::RunFinalize, /* useAppHostStreaming= */ false};
    static const TAppHostWalkerApplyNodeHandler walkerApplyPrepareScenarioHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::ApplyPrepareScenario, /* useAppHostStreaming= */ false};
    static const TAppHostWalkerApplyModifiersNodeHandler walkerApplysScenarioModifiersHandler{globalCtx, walker};
    static const TAppHostWalkerApplyNodeHandler walkerApplyRenderScenarioHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::ApplyRenderScenario, /* useAppHostStreaming= */ false};
    static const TAppHostWalkerPrepareNodeHandler walkerPrepareHandler{globalCtx, walker};
    static const TAppHostWalkerMonitoringNodeHandler walkerMonitoringHandler{globalCtx};
    static const TAppHostWalkerRunClassifyWinnerNodeHandler walkerRunClassifyWinnerHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::ClassifyWinner, /* useAppHostStreaming= */ false};
    static const TAppHostWalkerRunProcessCombinatorContinueNodeHandler walkerRunProcessCombinatorContinueHandler{globalCtx, walker, ILightWalkerRequestCtx::ERunStage::ProcessCombinatorContinue, /* useAppHostStreaming= */ false};

    auto addAsync = [&registry](const TString& path, const TAppHostNodeHandler& handler) {
        registry.Add(path, [&handler](NAppHost::TServiceContextPtr ctx) { return handler.RunAsync(ctx); });
    };

    auto addSync = [&registry](const TString& path, const TAppHostNodeHandler& handler) {
        registry.Add(path, [&handler](NAppHost::IServiceContext& ctx) { return handler.RunSync(ctx); });
    };

    addAsync("/mm_walker_apply_prepare_scenario", walkerApplyPrepareScenarioHandler);
    addAsync("/mm_walker_apply_scenario_modifiers", walkerApplysScenarioModifiersHandler);
    addAsync("/mm_walker_apply_render_scenario", walkerApplyRenderScenarioHandler);
    addAsync("/mm_walker_preclassify", preClassifyHandler);
    addAsync("/mm_walker_postclassify", walkerPostClassifyHandler);
    addAsync("/mm_walker_run_process_continue", walkerProcessContinueHandler);
    addAsync("/mm_walker_run_finalize", walkerRunFinalizeHandler);
    addAsync("/mm_walker_monitoring", walkerMonitoringHandler);
    addSync("/mm_walker_prepare", walkerPrepareHandler);
    addSync("/mm_walker_run_classify_winner", walkerRunClassifyWinnerHandler);
    addSync("/mm_walker_run_process_combinator_continue", walkerRunProcessCombinatorContinueHandler);
}

} // namespace NAlice::NMegamind
