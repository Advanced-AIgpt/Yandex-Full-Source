#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/nlg/register.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/play_tv_channel/player_setup.cpp>

namespace NAlice::NHollywood::NTvChannelsEfir {

REGISTER_SCENARIO("show_tv_channels_gallery",
                  AddHandle<TTvChannelsEfirPrepareHandle>()
                  .AddHandle<TTvChannelsEfirRenderHandle>()
                  .AddHandle<TPlayTvChannelPlayerSetup>()
                  .SetNlgRegistration(
                    NAlice::NHollywood::NLibrary::NScenarios::NTvChannelsEfir::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NTvChannelsEfir
