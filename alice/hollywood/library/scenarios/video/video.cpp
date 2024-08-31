#include "video_search.h"
#include "video_vh.h"

#include <alice/hollywood/library/scenarios/video/nlg/register.h>

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NVideo {

REGISTER_SCENARIO("video",   AddHandle<TVideoVhProxyPrepare>()
                            .AddHandle<TVideoVhPlayerGetLastProcess>()
                            .AddHandle<TVideoVhSeasonsProcess>()
                            .AddHandle<TVideoVhEpisodesProcess>()
                            .AddHandle<TVideoVhPlayerProcess>()
                            .AddHandle<TVideoSearchPrepare>()
                            .AddHandle<TVideoSearchProcess>()
                            .SetNlgRegistration(NLibrary::NScenarios::NVideo::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NVideo
