#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/blueprints/proto/blueprints.pb.h>

namespace NAlice::NHollywoodFw::NBlueprints {

class TBlueprintsScenario : public TScenario {
public:
    TBlueprintsScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    static TRetResponse RenderIrrelevant(const TBlueprintsRenderIrrelevant&, TRender&);
};

}  // namespace NAlice::NHollywood::NBlueprints