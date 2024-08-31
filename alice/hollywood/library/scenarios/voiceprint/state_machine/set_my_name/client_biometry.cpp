#include "set_my_name_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint_arguments.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TSetMyNameClientBiometryState::TSetMyNameClientBiometryState(TSetMyNameRunContext* context)
    : TSetMyNameRunStateBase(context)
{}

TSetMyNameRunStateBase::TRunHandleResult TSetMyNameClientBiometryState::HandleSetMyNameFrame(
    const TFrame& frame,
    TRunResponseBuilder& builder
)
{
    auto& logger = Context_->Logger();
    const auto& request = Context_->GetRequest();
    const auto* userInfo = Context_->UserInfo();

    const auto* guestData = GetGuestDataProto(request);
    const auto* guestOptions = GetGuestOptionsProto(request);
    if (!HasMatch(guestOptions)) {
        return Irrelevant();
    }

    if (!userInfo) {
        LOG_ERROR(logger) << "Failed to get uid from BlackBox user info";
        RenderResponse(builder, frame, /* isServerError = */ true);
        return {};
    }

    if (!guestOptions->HasYandexUID() || guestOptions->GetYandexUID().Empty()) {
        LOG_ERROR(logger) << "GuestOptions data source is present, but has empty YandexUID";
        RenderResponse(builder, frame, /* isServerError = */ true);
        return {};
    }

    if (!guestData) {
        LOG_WARN(logger) << "Has match, but GuestData data source is not present";
    }

    SetMyNameState_.SetOwnerUid(userInfo->GetUid());
    SetMyNameState_.SetGuestUid(guestOptions->GetYandexUID());
    SetMyNameState_.SetPersId(guestOptions->GetPersId());
    auto oldUserName = GetUserNameFromDataSync(request, SetMyNameState_.GetOwnerUid(), SetMyNameState_.GetGuestUid(), guestData, SetMyNameState_.GetPersId());
    if (oldUserName) {
        SetMyNameState_.SetOldUserName(*oldUserName);
    } else {
        LOG_WARN(logger) << "Failed to get old user's name from DataSync";
    }

    LOG_INFO(logger) << "Creating apply arguments to update user's name in DataSync";
    TVoiceprintArguments applyArgs;
    *applyArgs.MutableVoiceprintSetMyNameState() = SetMyNameState_;
    builder.SetApplyArguments(applyArgs);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
