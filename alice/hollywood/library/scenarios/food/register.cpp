#include "food_commit.h"
#include "food_run.h"

#include <alice/hollywood/library/scenarios/food/backend/auth.h>
#include <alice/hollywood/library/scenarios/food/backend/get_address.h>
#include <alice/hollywood/library/scenarios/food/backend/get_last_order_pa.h>
#include <alice/hollywood/library/scenarios/food/backend/get_menu_pa.h>
#include <alice/hollywood/library/scenarios/food/nlg/register.h>
#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NFood {

REGISTER_SCENARIO(
    "food",
    AddHandle<TRunPrepareHandle>()
    .AddHandle<TRunRenderHandle>()
    .AddHandle<NApiGetTaxiUid::TRequestHandle>()
    .AddHandle<NApiGetAddress::TRequestHandle>()
    .AddHandle<NApiGetLastOrderPA::TRequestHandle>()
    .AddHandle<NApiFindPlacePA::TRequestHandle>()
    .AddHandle<NApiGetMenuPA::TRequestHandle>()
    .AddHandle<TCommitDispatchHandle>()
    .AddHandle<TSyncCartProxyPrepareHandle>()
    .AddHandle<TSyncCartResponseHandle>()
    .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NFood::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NFood
