#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/find_poi/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("find_poi_tr",
                  AddHandle<TFindPoiTrPrepareHandle>()
                  .AddHandle<TFindPoiTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NFindPoi::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi
