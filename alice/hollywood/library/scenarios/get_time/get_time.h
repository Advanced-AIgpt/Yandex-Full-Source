#pragma once

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NGetTime {

class TGetTimeScenario : public TScenario {
public:
    TGetTimeScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    void Hook(THookInputInfo& info,
              NScenarios::TScenarioRunResponse& runResponse) const override;
};

}  // namespace NAlice::NHollywood::NGetTime
