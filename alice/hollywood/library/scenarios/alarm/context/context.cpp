#include "context.h"

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NReminders {

namespace {

TRemindersState ConstructRemindersState(const TScenarioRunRequestWrapper& runRequest) {
    TRemindersState state;
    const auto& rawState = runRequest.BaseRequestProto().GetState();
    if (rawState.Is<TRemindersState>() && !runRequest.IsNewSession()) {
        rawState.UnpackTo(&state);
    }

    return state;
}

} // namespace

TRemindersContext::TRemindersContext(TScenarioHandleContext& ctx)
    : Ctx_{&ctx}
    , RunRequestProto_{GetOnlyProtoOrThrow<TScenarioRunRequest>(Ctx_->ServiceCtx, REQUEST_ITEM)}
    , RunRequest_{RunRequestProto_, ctx.ServiceCtx}
    , RemindersState_{ConstructRemindersState(RunRequest_)}
    , Renderer_{std::make_unique<TRenderer>(ctx, RunRequest_)}
{
}

const TScenarioRunRequestWrapper& TRemindersContext::RunRequest() const {
    return RunRequest_;
}

TRenderer& TRemindersContext::Renderer() {
    return *Renderer_;
}

const TRemindersState& TRemindersContext::State() const {
    return RemindersState_;
}

TRemindersState& TRemindersContext::State() {
    return RemindersState_;
}

const TCallbackDirective* TRemindersContext::GetCallback() const {
    return RunRequest_.Input().GetCallback();
}

const TDeviceState& TRemindersContext::GetDeviceState() const {
    return RunRequest_.BaseRequestProto().GetDeviceState();
}

const NScenarios::TInterfaces& TRemindersContext::GetInterfaces() const {
    return RunRequest_.Proto().GetBaseRequest().GetInterfaces();
}

ui64 TRemindersContext::GetEpoch() const {
    return RunRequest_.ClientInfo().Epoch;
}

TString TRemindersContext::GetTimezone() const {
    return RunRequest_.ClientInfo().Timezone;
}

const TDeviceState_TAlarmState& TRemindersContext::GetAlarmState() const {
    return RunRequest_.BaseRequestProto().GetDeviceState().GetAlarmState();
}

const TDeviceState_TTimers& TRemindersContext::GetTimers() const {
    return RunRequest_.BaseRequestProto().GetDeviceState().GetTimers();
}

bool TRemindersContext::HasLedDisplay() const {
    return RunRequest_.Proto().GetBaseRequest().GetInterfaces().GetHasLedDisplay();
}

bool TRemindersContext::HasScledDisplay() const {
    return RunRequest_.Proto().GetBaseRequest().GetInterfaces().GetHasScledDisplay();
}

bool TRemindersContext::CanRenderDivCards() const {
    return RunRequest_.Proto().GetBaseRequest().GetInterfaces().GetCanRenderDivCards();
}

} // namespace NAlice::NHollywood::NReminders
