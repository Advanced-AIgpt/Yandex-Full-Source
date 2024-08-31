#include "set_my_name_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TSetMyNameBiometryDispatchState::TSetMyNameBiometryDispatchState(TSetMyNameRunContext* context)
    : TSetMyNameRunStateBase(context)
{}

TSetMyNameRunStateBase::TRunHandleResult TSetMyNameBiometryDispatchState::HandleSetMyNameFrame(
    const TFrame& frame,
    TRunResponseBuilder& builder
)
{
    ClearSetMyNameState();
    auto& logger = Context_->Logger();
    const auto& request = Context_->GetRequest();

    const auto distractorSlot = frame.FindSlot(SLOT_DISTRACTOR);
    if (distractorSlot) {
        LOG_INFO(logger) << "distractor slot is found. Rendering corresponding response...";
        AddSlot(RenderSlots_, SLOT_DISTRACTOR, true);
        RenderResponse(builder, frame, /* isServerError = */ false);
        return {};
    }

    const auto userNameSlot = frame.FindSlot(SLOT_USER_NAME);
    if (!userNameSlot) {
        LOG_WARN(logger) << "No user_name slot in frame " << frame.Name() << " found";
        return Irrelevant();
    }

    if (userNameSlot->Type == SWEAR_SLOT_TYPE) {
        LOG_INFO(logger) << "user_name slot is swear. Rendering corresponding response...";
        AddSlot(RenderSlots_, SLOT_SWEAR_USER_NAME, true);
        RenderResponse(builder, frame, /* isServerError = */ false);
        return {};
    }
    SetMyNameState_.SetUserName(userNameSlot->Value.AsString());

    std::unique_ptr<TSetMyNameRunStateBase> nextState = nullptr;
    if (IsBioCapabilitySupported(request)) {
        nextState = std::make_unique<TSetMyNameClientBiometryState>(Context_);
    } else {
        nextState = std::make_unique<TSetMyNameServerBiometryState>(Context_);
    }
    return {std::move(nextState), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
