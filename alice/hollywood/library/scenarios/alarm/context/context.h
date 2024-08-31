#pragma once

#include <alice/hollywood/library/scenarios/alarm/context/renderer.h>
#include <alice/hollywood/library/scenarios/alarm/proto/reminders.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NReminders {

class TRemindersContext {
public:
    TRemindersContext(TScenarioHandleContext& ctx);

    // wrappee context getters
    TScenarioHandleContext& Ctx() { return *Ctx_; }
    const TScenarioHandleContext& Ctx() const { return *Ctx_; }

    const TScenarioHandleContext* operator->() const { return Ctx_; }
    TScenarioHandleContext* operator->() { return Ctx_; }
    const TScenarioHandleContext& operator*() const { return *Ctx_; }
    TScenarioHandleContext& operator*() { return *Ctx_; }

    // request data functions
    TRTLogger& Logger() const { return Ctx_->Ctx.Logger(); }

    const TScenarioRunRequestWrapper& RunRequest() const;
    const NAlice::NScenarios::TCallbackDirective* GetCallback() const;
    const TDeviceState& GetDeviceState() const;
    const TDeviceState_TAlarmState& GetAlarmState() const;
    const TDeviceState_TTimers& GetTimers() const;
    const NScenarios::TInterfaces& GetInterfaces() const;
    ui64 GetEpoch() const;
    TString GetTimezone() const;

    bool HasLedDisplay() const;
    bool HasScledDisplay() const;
    bool CanRenderDivCards() const;

    // Alice answer renderer
    TRenderer& Renderer();

    // state functions
    const TRemindersState& State() const;
    TRemindersState& State();

private:
    TScenarioHandleContext* Ctx_;
    const NScenarios::TScenarioRunRequest RunRequestProto_;
    TScenarioRunRequestWrapper RunRequest_;

    TRemindersState RemindersState_;
    const std::unique_ptr<TRenderer> Renderer_;
};

} // namespace NAlice::NHollywood::NReminders
