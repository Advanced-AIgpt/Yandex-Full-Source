#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/test_scenario/proto/test_scenario.pb.h>

namespace NAlice::NHollywoodFw::NTestScenario {

class TTestScenarioScenario : public TScenario {
public:
    TTestScenarioScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    static TRetResponse RenderIrrelevant(const TTestScenarioRenderIrrelevant&, TRender&);
};

}  // namespace NAlice::NHollywood::NTestScenario
