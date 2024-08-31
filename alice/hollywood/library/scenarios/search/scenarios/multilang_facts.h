#pragma once

#include "base.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NSearch {

class TMultilangFactsScenario : public TSearchScenario {
public:
    using TSearchScenario::TSearchScenario;

    bool TryPrepare(NAppHost::IServiceContext& serviceCtx);
    bool TryRender(const NAppHost::IServiceContext& serviceCtx);

private:
    bool IsEnabled();
};

} // namespace NAlice::NHollywood::NSearch
