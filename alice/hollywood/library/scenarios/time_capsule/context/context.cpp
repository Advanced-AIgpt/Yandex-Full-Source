#include "context.h"

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTimeCapsule {

namespace {

TTimeCapsuleState ConstructTimeCapsuleState(const TScenarioRunRequestWrapper& runRequest) {
    TTimeCapsuleState state;
    const auto& rawState = runRequest.BaseRequestProto().GetState();
    if (rawState.Is<TTimeCapsuleState>() && !runRequest.IsNewSession()) {
        rawState.UnpackTo(&state);
    }

    return state;
}

} // namespace

TTimeCapsuleContext::TTimeCapsuleContext(TScenarioHandleContext& ctx)
    : Ctx_{&ctx}
    , RunRequestProto_{GetOnlyProtoOrThrow<TScenarioRunRequest>(Ctx_->ServiceCtx, REQUEST_ITEM)}
    , RunRequest_{RunRequestProto_, ctx.ServiceCtx}
    , TimeCapsuleState_{ConstructTimeCapsuleState(RunRequest_)}
    , MementoTimeCapsuleInfo_(RunRequest_.BaseRequestProto().GetMemento().GetUserConfigs().GetTimeCapsuleInfo())
{
}

const TScenarioRunRequestWrapper& TTimeCapsuleContext::RunRequest() const {
    return RunRequest_;
}

const NScenarios::TInterfaces& TTimeCapsuleContext::GetInterfaces() const {
    return RunRequest_.Proto().GetBaseRequest().GetInterfaces();
}

const TTimeCapsuleState& TTimeCapsuleContext::State() const {
    return TimeCapsuleState_;
}

const NMemento::TTimeCapsuleInfo& TTimeCapsuleContext::GetMementoTimeCapsuleInfo() const {
    return MementoTimeCapsuleInfo_;
}

bool TTimeCapsuleContext::IsSmartSpeaker() const {
    return RunRequest_.ClientInfo().IsSmartSpeaker();
}

} // namespace NAlice::NHollywood::NReminders
