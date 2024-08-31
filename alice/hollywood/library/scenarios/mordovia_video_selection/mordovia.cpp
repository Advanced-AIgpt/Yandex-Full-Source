#include "ott_setup.h"
#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/mordovia_video_selection/nlg/register.h>

namespace NAlice::NHollywood::NMordovia {

REGISTER_SCENARIO("mordovia_video_selection",
                  AddHandle<TMordoviaPrepareHandle>()
                  .AddHandle<TMordoviaRenderHandle>()
                  .AddHandle<TOttSetup>().SetNlgRegistration(
                      NAlice::NHollywood::NLibrary::NScenarios::NMordoviaVideoSelection::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NMordovia
