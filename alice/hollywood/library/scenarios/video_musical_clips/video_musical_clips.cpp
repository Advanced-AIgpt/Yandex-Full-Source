#include <alice/hollywood/library/scenarios/video_musical_clips/nlg/register.h>

#include "musical_clips_prepare_handle.h"
#include "musical_clips_render_handle.h"
#include "musical_clips_run_handle.h"
#include "search_clips_prepare_handle.h"
#include "vh_prepare_handle.h"

#include <alice/hollywood/library/registry/registry.h>


using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

REGISTER_SCENARIO("video_musical_clips",
                  AddHandle<NMusicalClips::TMusicalClipsPrepareHandle>()
                  .AddHandle<NMusicalClips::TSearchClipsPrepareHandle>()
                  .AddHandle<NMusicalClips::TVhPrepareHandle>()
                  .AddHandle<NMusicalClips::TMusicalClipsRunHandle>()
                  .AddHandle<NMusicalClips::TMusicalClipsRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NVideoMusicalClips::NNlg::RegisterAll));

} // namespace NAlice::NHollywood*/
