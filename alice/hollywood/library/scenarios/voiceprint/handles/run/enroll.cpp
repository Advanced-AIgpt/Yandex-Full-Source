#include "impl.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/state_machine/enroll_run/enroll_run.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TMaybe<NScenarios::TScenarioRunResponse> TRunHandleImpl::HandleEnroll(bool isEnrollmentSuggested) {
    auto& ctx = VoiceprintCtx_.Ctx;
    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), VoiceprintCtx_.Request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    bool isHandled = false;
    auto enrollmentRunCtx = TEnrollmentRunContext::MakeFrom(VoiceprintCtx_);
    const auto frameConfirm = VoiceprintCtx_.FindFrame(CONFIRM_FRAME);

    // NOTE: order matters!!!
    if (const auto frameEnrollNewGuest = VoiceprintCtx_.FindFrame(ENROLL_NEW_GUEST_FRAME)) {
        isHandled = enrollmentRunCtx->HandleEnrollNewGuestFrame(*frameEnrollNewGuest, builder);
    } else if (const auto frameEnrollGuestFinish = VoiceprintCtx_.FindFrame(ENROLL_GUEST_FINISH_FRAME)) {
        isHandled = enrollmentRunCtx->HandleGuestEnrollmentFinishFrame(*frameEnrollGuestFinish, builder);
    } else if (const auto frameSetMyName = VoiceprintCtx_.FindFrame(SET_MY_NAME_FRAME)) {
        isHandled = enrollmentRunCtx->HandleSetMyNameFrame(*frameSetMyName, builder);
    } else if (const auto frameEnroll = VoiceprintCtx_.FindFrame(ENROLL_FRAME)) {
        isHandled = enrollmentRunCtx->HandleEnrollFrame(*frameEnroll, builder);
    } else if (const auto frameEnrollReady = VoiceprintCtx_.FindFrame(ENROLL_READY_FRAME)) {
        isHandled = enrollmentRunCtx->HandleEnrollReadyFrame(*frameEnrollReady, builder);
    } else if (const auto frameEnrollCancel = VoiceprintCtx_.FindFrame(ENROLL_CANCEL_FRAME)) {
        isHandled = enrollmentRunCtx->HandleEnrollCancelFrame(*frameEnrollCancel, builder);
    } else if (const auto frameEnrollStart = VoiceprintCtx_.FindFrame(ENROLL_START_FRAME)) {
        isHandled = enrollmentRunCtx->HandleEnrollStartFrame(*frameEnrollStart, builder);
    } else if (const auto frameRepeat = VoiceprintCtx_.FindFrame(REPEAT_FRAME)) {
        isHandled = enrollmentRunCtx->HandleRepeatFrame(*frameRepeat, builder);
    } else if (isEnrollmentSuggested && frameConfirm) {
        isHandled = enrollmentRunCtx->HandleEnrollFrame(*frameConfirm, builder);
    } else if (IsCollectingPhrases(VoiceprintCtx_.ScenarioStateProto)) {
        auto emulatedFrame = TFrame::FromProto(TSemanticFrame{});
        emulatedFrame.SetName(TString(ENROLL_COLLECT_FRAME_EMULATED));
        isHandled = enrollmentRunCtx->HandleEnrollCollectFrame(emulatedFrame, builder);
    } else {
        return Nothing();
    }
    
    Y_ENSURE(isHandled);
    return *std::move(builder).BuildResponse();
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
