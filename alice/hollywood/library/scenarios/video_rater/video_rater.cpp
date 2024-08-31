#include "commit_prepare_handle.h"
#include "commit_render_handle.h"
#include "prepare_handle.h"
#include "render_handle.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/hollywood/library/scenarios/video_rater/nlg/register.h>

namespace NAlice::NHollywood::NVideoRater {

REGISTER_SCENARIO("video_rater",
                  AddHandle<TVideoRaterPrepareHandle>()
                  .AddHandle<TVideoRaterRenderHandle>()
                  .AddHandle<TVideoRaterCommitPrepareHandle>()
                  .AddHandle<TVideoRaterCommitRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NVideoRater::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NVideoRater
