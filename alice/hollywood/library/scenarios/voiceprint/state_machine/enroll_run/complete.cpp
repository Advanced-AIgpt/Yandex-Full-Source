#include "enroll_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint_arguments.pb.h>

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TEnrollmentCompleteState::TEnrollmentCompleteState(TEnrollmentRunContext* context)
    : TEnrollmentRunStateBase(context)
{}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    LOG_INFO(Context_->Logger()) << "Got " << frame.Name() << " frame. Need to fall back to NotStarted state";
    return {std::make_unique<TEnrollmentNotStartedState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    return FallBackToNotStarted(frame);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    CancelActiveEnrollment(frame, builder);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    return FallBackToNotStarted(frame);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    return FallBackToNotStarted(frame);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    return FallBackToNotStarted(frame);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    return FallBackToNotStarted(frame);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    return FallBackToNotStarted(frame);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    auto& logger = Context_->Logger();

    EnrollState_.SetCurrentStage(TVoiceprintEnrollState::Finish);

    if (EnrollState_.GetGuestPuid().Empty()) {
        LOG_ERROR(logger) << "Unexpectedly got " << frame.Name() << " frame in non-guest enrollment. "
                          << "Check SendGuestEnrollmentFinishFrame flag in TEnrollmentFinishDirective on previous request or report to valbon@";
        return ReportUnexpectedFrame(frame, builder);
    }

    LOG_INFO(logger) << "Creating apply arguments to finish guest enrollment";
    TVoiceprintArguments applyArgs;
    *applyArgs.MutableVoiceprintEnrollState() = EnrollState_;
    builder.SetApplyArguments(applyArgs);
    
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCompleteState::FallBackToNotStarted(const TFrame& frame) {
    LOG_WARN(Context_->Logger()) << "Unexpectedly got " << frame.Name() << " frame. Falling back to NotStarted state";
    return {std::make_unique<TEnrollmentNotStartedState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
