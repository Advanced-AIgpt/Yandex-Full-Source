#pragma once

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NReminders {

class TRemindersScenario : public TScenario {
public:
    TRemindersScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    void Hook(THookInputInfo& info,
              NScenarios::TScenarioRunResponse& runResponse) const override;

    void Hook(THookInputInfo& info,
              NScenarios::TScenarioApplyResponse& applyResponse) const override;
};

}  // namespace NAlice::NHollywood::NReminders
