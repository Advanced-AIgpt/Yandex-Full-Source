#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/show_route/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("show_route",
                  AddHandle<TShowRouteTrPrepareHandle>()
                  .AddHandle<TShowRouteTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NShowRoute::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi
