#include "remove_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/library/data_sync/data_sync.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TRemoveClientBiometryInitState::TRemoveClientBiometryInitState(TRemoveRunContext* context)
    : TRemoveRunStateBase(context)
{}

TRemoveRunStateBase::TRunHandleResult TRemoveClientBiometryInitState::HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    auto& logger = Context_->Logger();
    const auto& request = Context_->GetRequest();
    
    auto prepareResult = PrepareRemove();
    if (prepareResult == EPrepareResult::Success) {
        RemoveState_.SetCurrentStage(TVoiceprintRemoveState::WaitConfirm);
        RenderResponse(builder, frame, NLG_REMOVE_CONFIRM, /* expectsAnswer = */ true);
    } else {
        if (request.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_FALLBACK_TO_SERVER_BIOMETRY)) {
            LOG_INFO(logger) << "Fallback to server biometry was enabled, continue to handle frame in new state";
            return {std::make_unique<TRemoveServerBiometryInitState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
        }

        auto patchedFrame = frame;
        patchedFrame.SetName(TString(REMOVE_FINISH_FRAME));
        switch (prepareResult) {
            case EPrepareResult::NoMatch:
                {
                    bool suggestEnrollment = request.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_MULTIACCOUNT);
                    if (suggestEnrollment) {
                        AddSlot(RenderSlots_, SLOT_IS_ENROLLMENT_SUGGESTED, true);
                        Context_->ScenarioStateProto().SetIsEnrollmentSuggested(true);
                    }
                    RenderResponse(builder, patchedFrame, NLG_REMOVE_UNKNOWN_USER, suggestEnrollment);
                }
                break;
            case EPrepareResult::ServerError:
                NlgData_.Context["attentions"]["server_error"] = true;
                RenderResponse(builder, patchedFrame, NLG_REMOVE_FINISH, /* expectsAnswer = */ false);
                break;
            default:
                LOG_ERROR(logger) << "All EPrepareResult error cases are expected to be handled by this line, got: " << ToString(prepareResult);
                Y_UNREACHABLE();
        }
    }

    return {};
}

TRemoveClientBiometryInitState::EPrepareResult TRemoveClientBiometryInitState::PrepareRemove() {
    auto& logger = Context_->Logger();
    auto& request = Context_->GetRequest();
    const auto* userInfo = Context_->UserInfo();
    const auto clientInfo = request.ClientInfo();
    const auto* guestOptions = GetGuestOptionsProto(request);
    const auto* guestData = GetGuestDataProto(request);

    if (!userInfo) {
        LOG_ERROR(logger) << "Failed to get uid from BlackBox user info";
        return EPrepareResult::ServerError;
    }
    
    if (!guestOptions || guestOptions->GetStatus() == NAlice::TGuestOptions::NoMatch) {
        LOG_INFO(logger) << "No client biometry found in request";
        return EPrepareResult::NoMatch;
    }

    if (!guestOptions->HasYandexUID() || guestOptions->GetYandexUID().Empty()) {
        LOG_ERROR(logger) << "GuestOptions data source is present, but has empty YandexUID";
        return EPrepareResult::ServerError;
    }

    if (!guestOptions->HasPersId() || guestOptions->GetPersId().Empty()) {
        LOG_ERROR(logger) << "GuestOptions data source is present, but has empty PersId";
        return EPrepareResult::ServerError;
    }

    if (!guestData) {
        LOG_WARN(logger) << "Has match, but GuestData data source is not present";
    }

    RemoveState_.SetUid(guestOptions->GetYandexUID());
    RemoveState_.SetPersId(guestOptions->GetPersId());
    NlgData_.Context["attentions"]["biometry_guest"] = false;

    auto userName = GetUserNameFromDataSync(request, userInfo->GetUid(), RemoveState_.GetUid(), guestData, RemoveState_.GetPersId());

    if (userName) {
        RemoveState_.SetUserName(*userName);
    } else {
        LOG_WARN(logger) << "Failed to get user's name from DataSync";
    }

    return EPrepareResult::Success;
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
