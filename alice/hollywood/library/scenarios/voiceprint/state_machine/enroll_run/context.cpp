#include "enroll_run.h"

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint {

TEnrollmentRunContext::TEnrollmentRunContext(TVoiceprintHandleContext& voiceprintCtx)
    : TRunStateMachineContextBase<NImpl::TEnrollmentRunStateBase>(voiceprintCtx)
{}

std::unique_ptr<TEnrollmentRunContext> TEnrollmentRunContext::MakeFrom(TVoiceprintHandleContext& voiceprintCtx) {
    auto enrollmentCtx = std::unique_ptr<TEnrollmentRunContext>(new TEnrollmentRunContext(voiceprintCtx));
    auto curStage = voiceprintCtx.ScenarioStateProto.GetVoiceprintEnrollState().GetCurrentStage();
    switch (curStage) {
        case TVoiceprintEnrollState::NotStarted:
            enrollmentCtx->State_ = std::make_unique<NImpl::TEnrollmentNotStartedState>(enrollmentCtx.get());
            return std::move(enrollmentCtx);
        case TVoiceprintEnrollState::Intro:
            enrollmentCtx->State_ = std::make_unique<NImpl::TEnrollmentIntroState>(enrollmentCtx.get());
            return std::move(enrollmentCtx);
        case TVoiceprintEnrollState::WaitUsername:
            enrollmentCtx->State_ = std::make_unique<NImpl::TEnrollmentWaitUsernameState>(enrollmentCtx.get());
            return std::move(enrollmentCtx);
        case TVoiceprintEnrollState::WaitReady:
            enrollmentCtx->State_ = std::make_unique<NImpl::TEnrollmentWaitReadyState>(enrollmentCtx.get());
            return std::move(enrollmentCtx);
        case TVoiceprintEnrollState::Collect:
            enrollmentCtx->State_ = std::make_unique<NImpl::TEnrollmentCollectState>(enrollmentCtx.get());
            return std::move(enrollmentCtx);
        case TVoiceprintEnrollState::Complete:
            enrollmentCtx->State_ = std::make_unique<NImpl::TEnrollmentCompleteState>(enrollmentCtx.get());
            return std::move(enrollmentCtx);
        case TVoiceprintEnrollState::Finish:
            enrollmentCtx->State_ = std::make_unique<NImpl::TEnrollmentCompleteState>(enrollmentCtx.get());
            return std::move(enrollmentCtx);
        case TVoiceprintEnrollState_EStage_TVoiceprintEnrollState_EStage_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TVoiceprintEnrollState_EStage_TVoiceprintEnrollState_EStage_INT_MAX_SENTINEL_DO_NOT_USE_:
            LOG_ERROR(voiceprintCtx.Logger) << "Unexpected current stage: " << TVoiceprintEnrollState::EStage_Name(curStage);
            Y_UNREACHABLE();
    }
}

DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleEnrollNewGuestFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleEnrollFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleEnrollCancelFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleEnrollReadyFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleEnrollStartFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleEnrollCollectFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleRepeatFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleSetMyNameFrame)
DEFINE_CONTEXT_HANDLE_METHOD(TEnrollmentRunContext, HandleGuestEnrollmentFinishFrame)

const TFrame* TEnrollmentRunContext::FindFrame(TStringBuf frameName) {
    return VoiceprintCtx_.FindFrame(frameName);
}

const TSemanticFrame* TEnrollmentRunContext::FindSemanticFrame(TStringBuf frameName) {
    return VoiceprintCtx_.FindSemanticFrame(frameName);
}

} // namespace NAlice::NHollywood::NVoiceprint
