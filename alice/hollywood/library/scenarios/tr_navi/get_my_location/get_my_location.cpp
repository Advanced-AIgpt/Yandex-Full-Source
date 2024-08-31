#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/get_my_location/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("get_my_location_tr",
                  AddHandle<TGetMyLocationTrPrepareHandle>()
                  .AddHandle<TGetMyLocationTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NGetMyLocation::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi