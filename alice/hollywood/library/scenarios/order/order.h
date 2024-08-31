#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/order/proto/order.pb.h>

namespace NAlice::NHollywoodFw::NOrder {

class TOrderScenario : public TScenario {
public:
    TOrderScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;

    static TRetResponse RenderIrrelevant(const TOrderRenderIrrelevant&, TRender&);
};

}  // namespace NAlice::NHollywood::NOrder