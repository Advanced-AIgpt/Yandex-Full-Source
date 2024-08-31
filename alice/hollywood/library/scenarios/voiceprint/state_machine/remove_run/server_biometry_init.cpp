#include "remove_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/server_biometry.h>

#include <alice/hollywood/library/frame/frame.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TRemoveServerBiometryInitState::TRemoveServerBiometryInitState(TRemoveRunContext* context)
    : TRemoveRunStateBase(context)
{}

TRemoveRunStateBase::TRunHandleResult TRemoveServerBiometryInitState::HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    auto renderTemplate = PrepareRemove();
    auto expectsAnswer = false;
    auto frameForRender = frame;
    if (!renderTemplate) {
        renderTemplate = NLG_REMOVE_CONFIRM;
        expectsAnswer = true;
        RemoveState_.SetCurrentStage(TVoiceprintRemoveState::WaitConfirm);
    } else {
        frameForRender.SetName(TString(REMOVE_FINISH_FRAME));
    }
    RenderResponse(builder, frameForRender, renderTemplate, expectsAnswer);
    return {};
}

TStringBuf TRemoveServerBiometryInitState::PrepareRemove() {
    auto& logger = Context_->Logger();
    auto& request = Context_->GetRequest();
    const auto* userInfo = Context_->UserInfo();

    if (!userInfo) {
        LOG_ERROR(logger) << "Failed to get uid from BlackBox user info";
        NlgData_.Context["attentions"]["server_error"] = true;
        return NLG_REMOVE_FINISH;
    }

    auto biometryData = ProcessServerBiometry(logger, request, userInfo->GetUid(),
                                                    NBiometry::TBiometry::EMode::NoGuest,
                                                    /* addMaxAccuracyIncognitoCheck = */ true);
    if (!biometryData) {
        return NLG_REMOVE_UNKNOWN_USER;
    }

    RemoveState_.SetUid(biometryData->Uid);
    RemoveState_.SetUserName(biometryData->UserName);
    NlgData_.Context["attentions"]["biometry_guest"] = biometryData->IsIncognitoUser;

    return "";
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
