#include "enroll_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>

#include <alice/hollywood/library/frame/frame.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

TEnrollmentNotStartedState::TEnrollmentNotStartedState(TEnrollmentRunContext* context)
    : TEnrollmentRunStateBase(context)
{}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    ClearEnrollState();
    auto& logger = Context_->Logger();

    auto renderTemplate = PrepareEnroll(/* isChangeNameRequest = */ false, /* isNewGuestFrame = */ true);
    TString extraDirective;
    if (!renderTemplate) {
        auto newGuest = Context_->FindSemanticFrame(frame.Name());
        Y_ENSURE(newGuest, "expected to get non-null GuestEnrollment TSF");
        const auto puid = newGuest->GetTypedSemanticFrame().GetGuestEnrollmentStartSemanticFrame().GetPuid().GetStringValue();
        if (!puid) {
            LOG_ERROR(logger) << "No puid in GuestEnrollmentStartSemanticFrame";
            renderTemplate = NLG_ENROLL_FINISH;
            AddSlot(RenderSlots_, NLU_SLOT_IS_SERVER_ERROR, true);
        } else {
            if (puid != EnrollState_.GetUid()) {
                EnrollState_.SetGuestPuid(puid);
            } else {
                LOG_INFO(logger) << "Got " << frame.Name() << " frame for owner. Do not set GuestPuid in scenario state";
            }
            AddSlot(RenderSlots_, "is_guest_push", true);

            renderTemplate = NLG_ENROLL;

            EnrollState_.SetCurrentStage(TVoiceprintEnrollState::Intro);
            extraDirective = "player_pause";

            AddSlot(RenderSlots_, "is_need_explain", true);
        }
    }

    RenderResponse(builder, frame, renderTemplate, extraDirective, nullptr, false);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return HandleFrameWithoutUserName(frame, builder, /* isChangeNameRequest = */ false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleEnrollCancelFrame(const TFrame& /* frame */, TRunResponseBuilder& builder) {
    Irrelevant(Context_->Logger(), Context_->GetRequest(), builder, EIrrelevantType::NotActive);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    const auto userNameSlot = frame.FindSlot(NLU_SLOT_USER_NAME);
    Y_ENSURE(userNameSlot, TStringBuilder{} << frame.Name() << " frame is expected to have " << NLU_SLOT_USER_NAME << " slot");
    return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ false, userNameSlot);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    const auto userNameSlot = frame.FindSlot(NLU_SLOT_USER_NAME);
    if (userNameSlot) {
        return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ true, userNameSlot);
    } else {
        return HandleFrameWithoutUserName(frame, builder, /* isChangeNameRequest = */ true);
    }
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleFrameWithUserName(
    const TFrame& frame,
    TRunResponseBuilder& builder,
    bool isChangeNameRequest,
    const TPtrWrapper<TSlot>& userNameSlot
)
{
    ClearEnrollState();

    auto renderTemplate = PrepareEnroll(isChangeNameRequest, /* isNewGuestFrame = */ false);
    TString extraDirective;
    auto patchedFrame = frame;

    if (!CheckSwear(userNameSlot)) {
        renderTemplate = NLG_ENROLL_FINISH;
    }

    if (!renderTemplate) {
        renderTemplate = NLG_ENROLL_COLLECT;
        if (!isChangeNameRequest) {
            patchedFrame.SetName(TString(ENROLL_COLLECT_FRAME));
        }

        EnrollState_.SetUserName(userNameSlot->Value.AsString());
        EnrollState_.SetCurrentStage(TVoiceprintEnrollState::Intro);
        extraDirective = "player_pause";
        
        AddSlot(RenderSlots_, "is_need_explain", true);
    }

    RenderResponse(builder, patchedFrame, renderTemplate, extraDirective, nullptr, false);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentNotStartedState::HandleFrameWithoutUserName(
    const TFrame& frame,
    TRunResponseBuilder& builder,
    bool isChangeNameRequest
)
{
    ClearEnrollState();

    auto renderTemplate = PrepareEnroll(isChangeNameRequest, /* isNewGuestFrame = */ false);
    TString extraDirective;
    if (!renderTemplate) {
        renderTemplate = NLG_ENROLL;
        EnrollState_.SetCurrentStage(TVoiceprintEnrollState::Intro);
        extraDirective = "player_pause";
        
        AddSlot(RenderSlots_, "is_need_explain", true);
    }

    RenderResponse(builder, frame, renderTemplate, extraDirective, nullptr, false);
    return {};
}

TStringBuf TEnrollmentNotStartedState::PrepareEnroll(bool isChangeNameRequest, bool isNewGuestFrame) {
    const auto* userInfo = Context_->UserInfo();

    TString uid = EnrollState_.GetUid();
    if (!uid && userInfo) {
        uid = userInfo->GetUid();
        EnrollState_.SetUid(uid);
    }

    EnrollState_.SetIsBioCapabilitySupported(IsBioCapabilitySupported(Context_->GetRequest()));
    
    if (CheckPrerequisites(isChangeNameRequest, isNewGuestFrame)) {
        return "";
    } else {
        return NLG_ENROLL_FINISH;
    }
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
