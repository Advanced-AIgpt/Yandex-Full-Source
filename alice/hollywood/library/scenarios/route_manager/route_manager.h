#pragma once

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NRouteManager {

//
// Диспетчер сценария
//
class TRouteManagerScenario : public TScenario {
public:
    TRouteManagerScenario();

    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const;
};

}  // namespace NAlice::NHollywoodFw::NRouteManager
