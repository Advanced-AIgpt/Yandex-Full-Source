#pragma once

#include <alice/hollywood/library/scenarios/time_capsule/proto/time_capsule.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/memento/proto/user_configs.pb.h>

namespace NAlice::NHollywood::NTimeCapsule {

namespace NMemento = ru::yandex::alice::memento::proto;

class TTimeCapsuleContext {
public:
    TTimeCapsuleContext(TScenarioHandleContext& ctx);

    // wrapper context getters
    const TScenarioHandleContext& Ctx() const { return *Ctx_; }
    const TScenarioHandleContext* operator->() const { return Ctx_; }
    const TScenarioHandleContext& operator*() const { return *Ctx_; }

    // request data functions
    TRTLogger& Logger() const { return Ctx_->Ctx.Logger(); }

    const TScenarioRunRequestWrapper& RunRequest() const;
    const NScenarios::TInterfaces& GetInterfaces() const;

    // state functions
    const TTimeCapsuleState& State() const;

    const NMemento::TTimeCapsuleInfo& GetMementoTimeCapsuleInfo() const;

    bool IsSmartSpeaker() const;

private:
    const TScenarioHandleContext* Ctx_;
    const NScenarios::TScenarioRunRequest RunRequestProto_;
    const TScenarioRunRequestWrapper RunRequest_;

    const TTimeCapsuleState TimeCapsuleState_;
    const NMemento::TTimeCapsuleInfo MementoTimeCapsuleInfo_;
};

} // namespace NAlice::NHollywood::NTimeCapsule
