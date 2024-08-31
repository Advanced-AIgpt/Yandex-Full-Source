#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/add_point/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("add_point_tr",
                  AddHandle<TAddPointTrPrepareHandle>()
                  .AddHandle<TAddPointTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NAddPoint::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi
