#include "prepare_with_bass.h"
#include "render_with_bass.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/show_traffic_bass/nlg/register.h>

namespace NAlice::NHollywood {

    REGISTER_SCENARIO("show_traffic_bass", AddHandle<TBassShowTrafficPrepareHandle>()
                                           .AddHandle<TBassShowTrafficRenderHandle>()
                                           .SetNlgRegistration(
                                               NAlice::NHollywood::NLibrary::NScenarios::NShowTrafficBass::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
