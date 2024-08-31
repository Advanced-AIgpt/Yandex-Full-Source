#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/navi_external_confirmation/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("navi_external_confirmation_tr",
                  AddHandle<TNaviExternalConfirmationTrPrepareHandle>()
                  .AddHandle<TNaviExternalConfirmationTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NNaviExternalConfirmation::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi
