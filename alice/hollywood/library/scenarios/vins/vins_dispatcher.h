#pragma once

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NVins {

class TVinsScenario : public TScenario {
public:
    TVinsScenario();

    TRetScene Dispatch(const TRunRequest& request,
                       const TStorage& storage,
                       const TSource& source) const;

    void Hook(THookInputInfo& info,
              NScenarios::TScenarioRunResponse& runResponse) const override;

    void Hook(THookInputInfo& info,
              NScenarios::TScenarioApplyResponse& applyResponse) const override;
};

}  // namespace NAlice::NHollywoodFw::NVins
