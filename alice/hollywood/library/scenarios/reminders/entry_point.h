#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NReminders {

inline const TString ON_PERMISSION_SUCCESS_CB_NAME = "reminders_premission_on_success_callback";
inline const TString ON_PERMISSION_FAIL_CB_NAME = "reminders_premission_on_fail_callback";
inline const TString ON_SUCCESS_CB_NAME = "reminders_on_success_callback";
inline const TString ON_FAIL_CB_NAME = "reminders_on_fail_callback";
inline const TString ON_SHOOT_FRAME = "alice.reminders.on_shoot";
inline const TString ON_CANCEL_FRAME = "alice.reminders.cancel";
inline const TString ON_LIST_FRAME = "alice.reminders.list";

class TRemindersEntryPointHandler : public TScenario::THandleBase {
public:
    TString Name() const override;
    void Do(TScenarioHandleContext& ctx) const override;
public:
    static const TVector<TStringBuf>& GetSupportedFrames();
    static bool IsCallbackSupported(const TStringBuf callbackName);
};

}  // namespace NAlice::NHollywood::NReminders
