#include "set_my_name_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint_arguments.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/server_biometry.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/library/biometry/biometry.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TSetMyNameServerBiometryState::TSetMyNameServerBiometryState(TSetMyNameRunContext* context)
    : TSetMyNameRunStateBase(context)
{}

TSetMyNameRunStateBase::TRunHandleResult TSetMyNameServerBiometryState::HandleSetMyNameFrame(
    const TFrame& frame,
    TRunResponseBuilder& builder
)
{
    auto& logger = Context_->Logger();
    const auto& request = Context_->GetRequest();
    const auto* userInfo = Context_->UserInfo();

    if (!userInfo) {
        LOG_ERROR(logger) << "Failed to get uid from BlackBox user info";
        RenderResponse(builder, frame, /* isServerError = */ true);
        return {};
    }
    auto uid = userInfo->GetUid();

    auto biometry = ProcessServerBiometry(logger, request, uid,
                                                NBiometry::TBiometry::EMode::HighTPR,
                                                /* addMaxAccuracyIncognitoCheck = */ false);
    if (!biometry || biometry->IsIncognitoUser) {
        return Irrelevant();
    }

    SetMyNameState_.SetOwnerUid(uid);
    if (!biometry->UserName.Empty()) {
        SetMyNameState_.SetOldUserName(biometry->UserName);
    }

    LOG_INFO(logger) << "Creating apply arguments to update user's name in DataSync";
    TVoiceprintArguments applyArgs;
    *applyArgs.MutableVoiceprintSetMyNameState() = SetMyNameState_;
    builder.SetApplyArguments(applyArgs);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
