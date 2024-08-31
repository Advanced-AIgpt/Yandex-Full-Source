#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/state_machine/set_my_name/set_my_name_run.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TMaybe<NScenarios::TScenarioRunResponse> TRunHandleImpl::HandleSetMyName() {
    auto frame = VoiceprintCtx_.FindFrame(SET_MY_NAME_FRAME);
    if (!frame) {
        return Nothing();
    }

    auto& ctx = VoiceprintCtx_.Ctx;
    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), VoiceprintCtx_.Request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    auto setMyNameRunCtx = TSetMyNameRunContext::MakeFrom(VoiceprintCtx_);
    if (!setMyNameRunCtx->HandleSetMyNameFrame(*frame, builder)) {
        return Nothing();
    }

    return *std::move(builder).BuildResponse();
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
