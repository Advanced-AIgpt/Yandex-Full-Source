#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/state_machine/remove_run/remove_run.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

bool IsRemovementInProgress(const TVoiceprintState& scState) {
    return scState.HasVoiceprintRemoveState() && scState.GetVoiceprintRemoveState().GetCurrentStage() != TVoiceprintRemoveState::NotStarted;
}

} // namespace

TMaybe<NScenarios::TScenarioRunResponse> TRunHandleImpl::HandleRemove() {
    if (!VoiceprintCtx_.Request.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_REMOVE)) {
        return Nothing();
    }

    auto& ctx = VoiceprintCtx_.Ctx;
    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), VoiceprintCtx_.Request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    bool isHandled = false;
    auto removeRunCtx = TRemoveRunContext::MakeFrom(VoiceprintCtx_);
    if (const auto frameRemove = VoiceprintCtx_.FindFrame(REMOVE_FRAME)) {
        isHandled = removeRunCtx->HandleRemoveFrame(*frameRemove, builder);
    } else if (IsRemovementInProgress(VoiceprintCtx_.ScenarioStateProto)) {
        auto emulatedFrame = TFrame::FromProto(TSemanticFrame{});
        emulatedFrame.SetName(TString(REMOVE_CANCEL_FRAME_EMULATED));
        isHandled = removeRunCtx->HandleCancelFrame(emulatedFrame, builder);
    } else {
        return Nothing();
    }

    Y_ENSURE(isHandled);
    return *std::move(builder).BuildResponse();
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
