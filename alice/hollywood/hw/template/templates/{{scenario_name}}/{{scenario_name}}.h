#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/{{scenario_name}}/proto/{{scenario_name}}.pb.h>

namespace NAlice::NHollywoodFw::N{{ScenarioName}} {

class T{{ScenarioName}}Scenario : public TScenario {
public:
    T{{ScenarioName}}Scenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    static TRetResponse RenderIrrelevant(const T{{ScenarioName}}RenderIrrelevant&, TRender&);
};

}  // namespace NAlice::NHollywood::N{{ScenarioName}}
