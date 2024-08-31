#include "enroll_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TEnrollmentIntroState::TEnrollmentIntroState(TEnrollmentRunContext* context)
    : TEnrollmentRunStateBase(context)
{}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    LOG_INFO(Context_->Logger()) << "Got " << frame.Name() << " frame. Need to fall back to NotStarted state";
    return {std::make_unique<TEnrollmentNotStartedState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return TransitToWaitState(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    CancelActiveEnrollment(frame, builder);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return TransitToWaitState(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    // TODO(klim-roma): Consider cancelling voiceprint scenario + redirecting to another scenario
    return TransitToWaitState(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    AddSlot(RenderSlots_, "is_need_explain", true);
    if (!CheckPrerequisites()) {
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, false);
        return {};
    }

    auto renderTemplate = NLG_ENROLL;   // TODO(klim-roma): Consider choosing NLG_ENROLL_COLLECT in case of non-empty name
    RenderRepeat(frame, builder, frame.Name(), renderTemplate, false);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ true);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::HandleFrameWithUserName(
    const TFrame& frame,
    TRunResponseBuilder& builder,
    bool isChangeNameRequest
)
{
    if (!CheckPrerequisites(isChangeNameRequest)) {
        AddSlot(RenderSlots_, "is_need_explain", true);
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, false);
        return {};
    }
    
    auto& logger = Context_->Logger();
    
    EnrollState_.SetCurrentStage(TVoiceprintEnrollState::WaitUsername);

    LOG_INFO(logger) << "Continue to handle " << frame.Name() << " frame in WaitUsername state";
    return {std::make_unique<TEnrollmentWaitUsernameState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentIntroState::TransitToWaitState(const TFrame& frame, TRunResponseBuilder& builder) {
    if (!CheckPrerequisites()) {
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, false);
        return {};
    }

    auto& logger = Context_->Logger();

    if (EnrollState_.GetUserName()) {
        LOG_INFO(logger) << "User's name is found in scenario state. Continue to handle " << frame.Name() << " frame in WaitReady state";
        EnrollState_.SetCurrentStage(TVoiceprintEnrollState::WaitReady);
        return {std::make_unique<TEnrollmentWaitReadyState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
    }

    EnrollState_.SetCurrentStage(TVoiceprintEnrollState::WaitUsername);
    auto patchedFrame = frame;
    patchedFrame.SetName(TString(ENROLL_COLLECT_FRAME));
    AddSlot(RenderSlots_, "phrases_count", 0);
    
    RenderResponse(builder, patchedFrame, NLG_ENROLL_COLLECT, "", nullptr, false);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
