#include "remove_run.h"

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint {

TRemoveRunContext::TRemoveRunContext(TVoiceprintHandleContext& voiceprintCtx)
    : TRunStateMachineContextBase<NImpl::TRemoveRunStateBase>(voiceprintCtx)
{}

std::unique_ptr<TRemoveRunContext> TRemoveRunContext::MakeFrom(TVoiceprintHandleContext& voiceprintCtx) {
    auto removeCtx = std::unique_ptr<TRemoveRunContext>(new TRemoveRunContext(voiceprintCtx));
    auto curStage = voiceprintCtx.ScenarioStateProto.GetVoiceprintRemoveState().GetCurrentStage();
    switch (curStage) {
        case TVoiceprintRemoveState::NotStarted:
        case TVoiceprintRemoveState::Finish:
            removeCtx->State_ = std::make_unique<NImpl::TRemoveNotStartedState>(removeCtx.get());
            return std::move(removeCtx);
        case TVoiceprintRemoveState::WaitConfirm:
            removeCtx->State_ = std::make_unique<NImpl::TRemoveWaitConfirmState>(removeCtx.get());
            return std::move(removeCtx);
        case TVoiceprintRemoveState_EStage_TVoiceprintRemoveState_EStage_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TVoiceprintRemoveState_EStage_TVoiceprintRemoveState_EStage_INT_MAX_SENTINEL_DO_NOT_USE_:
            LOG_ERROR(voiceprintCtx.Logger) << "Unexpected current stage: " << TVoiceprintRemoveState::EStage_Name(curStage);
            Y_UNREACHABLE();
    }
}

DEFINE_CONTEXT_HANDLE_METHOD(TRemoveRunContext, HandleRemoveFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TRemoveRunContext, HandleCancelFrame)

} // namespace NAlice::NHollywood::NVoiceprint
