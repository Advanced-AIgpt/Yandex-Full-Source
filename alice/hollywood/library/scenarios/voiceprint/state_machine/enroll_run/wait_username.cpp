#include "enroll_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TEnrollmentWaitUsernameState::TEnrollmentWaitUsernameState(TEnrollmentRunContext* context)
    : TEnrollmentRunStateBase(context)
{}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    LOG_INFO(Context_->Logger()) << "Got " << frame.Name() << " frame. Need to fall back to NotStarted state";
    return {std::make_unique<TEnrollmentNotStartedState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CheckAndRepeat(frame, builder, /* isServerRepeat= */ true);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    CancelActiveEnrollment(frame, builder);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CheckAndRepeat(frame, builder, /* isServerRepeat= */ false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    const auto userNameSlot = frame.FindSlot(NLU_SLOT_USER_NAME);
    Y_ENSURE(userNameSlot, TStringBuilder{} << frame.Name() << " frame is expected to have " << NLU_SLOT_USER_NAME << " slot");
    return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ false, userNameSlot);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    // TODO(klim-roma): Consider cancelling voiceprint scenario + redirecting to another scenario
    return CheckAndRepeat(frame, builder, /* isServerRepeat= */ true);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CheckAndRepeat(frame, builder, /* isServerRepeat= */ false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    const auto userNameSlot = frame.FindSlot(NLU_SLOT_USER_NAME);
    if (userNameSlot) {
        return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ true, userNameSlot);
    } else {
        return CheckAndRepeat(frame, builder, /* isServerRepeat= */ true);
    }
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitUsernameState::HandleFrameWithUserName(
    const TFrame& frame,
    TRunResponseBuilder& builder,
    bool isChangeNameRequest,
    const TPtrWrapper<TSlot>& userNameSlot
)
{
    if (!CheckPrerequisites(isChangeNameRequest) || !CheckSwear(userNameSlot)) {
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, false);
        return {};
    }

    EnrollState_.SetUserName(userNameSlot->Value.AsString());
    EnrollState_.SetCurrentStage(TVoiceprintEnrollState::WaitReady);

    auto patchedFrame = frame;
    patchedFrame.SetName(TString(ENROLL_COLLECT_FRAME));
    RenderResponse(builder, patchedFrame, NLG_ENROLL_COLLECT, "", nullptr, false);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
