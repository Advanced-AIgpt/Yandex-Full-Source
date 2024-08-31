#include "remove_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint_arguments.pb.h>

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TRemoveWaitConfirmState::TRemoveWaitConfirmState(TRemoveRunContext* context)
    : TRemoveRunStateBase(context)
{}

TRemoveRunStateBase::TRunHandleResult TRemoveWaitConfirmState::HandleRemoveFrame(const TFrame& /* frame */, TRunResponseBuilder& builder) {
    auto& logger = Context_->Logger();
    LOG_INFO(logger) << "Creating apply arguments to finish removement";
    TVoiceprintArguments applyArgs;
    *applyArgs.MutableVoiceprintRemoveState() = RemoveState_;
    builder.SetApplyArguments(applyArgs);
    return {};
}

TRemoveRunStateBase::TRunHandleResult TRemoveWaitConfirmState::HandleCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    AddSlot(RenderSlots_, "is_removed", false);
    RenderResponse(builder, frame, NLG_REMOVE_FINISH, /* expectsAnswer = */ false);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
