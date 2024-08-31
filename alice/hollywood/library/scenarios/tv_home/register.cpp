#include "tv_home_run.h"
#include "tv_home_continue.h"
#include "tv_home_render.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/tv_home/nlg/register.h>

namespace NAlice::NHollywood {

REGISTER_SCENARIO(
    "tv_home",
    AddHandle<TTvHomeRunHandle>()
        .AddHandle<TTvHomeContinueHandle>()
        .AddHandle<TTvHomeRenderHandle>()
        .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTvHome::NNlg::RegisterAll)
);

} // namespace NAlice::NHollywood
