#include <alice/hollywood/library/scenarios/voiceprint/handles/run/main.h>
#include <alice/hollywood/library/scenarios/voiceprint/handles/apply_prepare/main.h>
#include <alice/hollywood/library/scenarios/voiceprint/handles/apply_render/main.h>
#include <alice/hollywood/library/scenarios/voiceprint/nlg/register.h>

#include <alice/hollywood/library/registry/registry.h>

namespace NAlice::NHollywood::NVoiceprint {

REGISTER_SCENARIO("voiceprint",
                  AddHandle<TRunHandle>()
                  .AddHandle<TApplyPrepareHandle>()
                  .AddHandle<TApplyRenderHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NVoiceprint::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::NVoiceprint
