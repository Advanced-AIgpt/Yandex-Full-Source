#pragma once
#include "base.h"

namespace NAlice::NHollywood::NSearch {

class TNavigatorScenario : public TSearchScenario {
public:
    using TSearchScenario::TSearchScenario;

    bool Do(const TSearchResult& response) override;
};

} // namespace NAlice::NHollywood
