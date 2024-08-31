#include "enroll_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint_arguments.pb.h>

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TEnrollmentCollectState::TEnrollmentCollectState(TEnrollmentRunContext* context)
    : TEnrollmentRunStateBase(context)
{}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    LOG_INFO(Context_->Logger()) << "Got " << frame.Name() << " frame. Need to fall back to NotStarted state";
    return {std::make_unique<TEnrollmentNotStartedState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CollectPhrase(frame, builder, false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    CancelActiveEnrollment(frame, builder);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CollectPhrase(frame, builder, false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CollectPhrase(frame, builder, false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CollectPhrase(frame, builder, false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CollectPhrase(frame, builder, true);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CollectPhrase(frame, builder, false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentCollectState::CollectPhrase(const TFrame& frame, TRunResponseBuilder& builder, bool isRepeat) {
    if (!CheckPrerequisites()) {
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, true);
        return {};
    }

    auto& logger = Context_->Logger();
    auto& request = Context_->GetRequest();

    TString forceFrameName;
    TStringBuf renderTemplate;
    TString requestId;
    auto& input = request.Input();
    if (input.IsVoiceInput()) {
        requestId = input.Proto().GetVoice().GetBiometryScoring().GetRequestId();
    }
    auto phrasesCount = EnrollState_.GetPhrasesCount();
    if (requestId) {        
        *EnrollState_.MutableRequestIds()->Add() = requestId;
        if (!isRepeat) {
            ++phrasesCount;
        }
        if (phrasesCount < MAX_PHARSES) {
            EnrollState_.SetPhrasesCount(phrasesCount);
            forceFrameName = ENROLL_COLLECT_FRAME;
            renderTemplate = NLG_ENROLL_COLLECT;
        } else {
            AddSlot(RenderSlots_, "created_uid", true);
            EnrollState_.SetCurrentStage(TVoiceprintEnrollState::Complete);

            LOG_INFO(logger) << "Creating apply arguments to finish enrollment";
            TVoiceprintArguments applyArgs;
            *applyArgs.MutableVoiceprintEnrollState() = EnrollState_;
            builder.SetApplyArguments(applyArgs);
            return {};
        }
    } else {
        LOG_WARNING(logger) << "No biometry scoring data found";
        AddSlot(RenderSlots_, "is_server_repeat", true);
        forceFrameName = ENROLL_COLLECT_FRAME;
        renderTemplate = NLG_ENROLL_COLLECT;
    }
    AddSlot(RenderSlots_, "phrases_count", phrasesCount);

    auto patchedFrame = frame;
    patchedFrame.SetName(forceFrameName);
    RenderResponse(builder, patchedFrame, renderTemplate, "", nullptr, true);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
