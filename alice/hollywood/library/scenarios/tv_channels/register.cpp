#include "common.h"
#include "prepare.h"
#include "render.h"

#include <alice/hollywood/library/scenarios/tv_channels/nlg/register.h>

#include <alice/hollywood/library/registry/registry.h>

#include <library/cpp/iterator/mapped.h>

namespace NAlice::NHollywood::NTvChannels {

REGISTER_SCENARIO("tv_channels",
                  AddHandle<TSwitchTvChannelPrepareHandle>()
                      .AddHandle<TSwitchTvChannelRenderHandle>()
                      .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTvChannels::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTvChannels
