#include "remove_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/protos/endpoint/capabilities/bio/capability.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TRemoveNotStartedState::TRemoveNotStartedState(TRemoveRunContext* context)
    : TRemoveRunStateBase(context)
{}

TRemoveRunStateBase::TRunHandleResult TRemoveNotStartedState::HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    auto& logger = Context_->Logger();
    auto& request = Context_->GetRequest();
    const auto clientInfo = request.ClientInfo();
    RemoveState_.SetIsBioCapabilitySupported(IsBioCapabilitySupported(request));

    if (RemoveState_.GetIsBioCapabilitySupported()) {
        LOG_INFO(logger) << "BioCapability is enabled and supported. Continue to handle " << frame.Name() << " in ClientBiometryInit state";
        return {std::make_unique<TRemoveClientBiometryInitState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
    } else {
        LOG_INFO(logger) << "BioCapability isn't supported. Continue to handle " << frame.Name() << " in ServerBiometryInit state";
        return {std::make_unique<TRemoveServerBiometryInitState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
    }
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
