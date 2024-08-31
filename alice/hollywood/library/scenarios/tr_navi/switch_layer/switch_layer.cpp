#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/tr_navi/switch_layer/nlg/register.h>

namespace NAlice::NHollywood::NTrNavi {

REGISTER_SCENARIO("switch_layer_tr",
                  AddHandle<TSwitchLayerTrPrepareHandle>()
                  .AddHandle<TSwitchLayerTrRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTrNavi::NSwitchLayer::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTrNavi
