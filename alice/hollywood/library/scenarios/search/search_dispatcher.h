#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/search/proto/search.pb.h>

#include <alice/hollywood/library/scenarios/search/scenes/screendevice_scene.h>
#include <alice/hollywood/library/scenarios/search/scenes/old_flow.h>

namespace NAlice::NHollywoodFw::NSearch {

class TSearchScenario: public TScenario {
public:
    TSearchScenario();
    TRetScene Dispatch(const TRunRequest& request, const TStorage& storage, const TSource& source) const;

private:
    void Hook(THookInputInfo& info, NScenarios::TScenarioRunResponse& runResponse) const override;
};

} // namespace NAlice::NHollywoodFw::NSearch
