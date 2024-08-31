#include "enroll_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>

#include <alice/hollywood/library/frame/frame.h>

#include <alice/protos/data/scenario/voiceprint/personalization_data.pb.h>

#include <util/generic/guid.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

constexpr uint32_t WHOLE_TIMEOUT_MS = 600 * 1000;

void AddEnrollmentStartDirective(TRTLogger& logger, const TVoiceprintEnrollState& enrollState, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TDirective directive;
    auto& enrollStartDirective = *directive.MutableEnrollmentStartDirective();
    enrollStartDirective.SetPersId(enrollState.GetPersId());
    enrollStartDirective.SetTimeoutMs(WHOLE_TIMEOUT_MS);
    if (!enrollState.GetGuestPuid().Empty()) {
        enrollStartDirective.SetPuid(FromString<uint64_t>(enrollState.GetGuestPuid()));
    } else {
        enrollStartDirective.SetPuid(FromString<uint64_t>(enrollState.GetUid()));
    }
    // may be set another one request timeout
    LOG_INFO(logger) << "AddEnrollmentStartDirective: " << directive;
    bodyBuilder.AddDirective(std::move(directive));
    bodyBuilder.AddTtsPlayPlaceholderDirective();
}

TString GeneratePersId() {
    const auto& guid = CreateGuidAsString();
    return TStringBuilder() << "PersId-" << guid;
}

} // namespace

TEnrollmentWaitReadyState::TEnrollmentWaitReadyState(TEnrollmentRunContext* context)
    : TEnrollmentRunStateBase(context)
{}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& /* builder */) {
    LOG_INFO(Context_->Logger()) << "Got " << frame.Name() << " frame. Need to fall back to NotStarted state";
    return {std::make_unique<TEnrollmentNotStartedState>(Context_), /* continueProcessing = */ true, /* isIrrelevant = */ false};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CheckAndRepeat(frame, builder, /* isServerRepeat= */ true);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    CancelActiveEnrollment(frame, builder);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    if (!CheckPrerequisites()) {
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, false);
        return {};
    }

    auto& logger = Context_->Logger();

    TString renderTemplate = TString(NLG_ENROLL_COLLECT);
    EnrollmentDirectiveFunc directiveFunc = nullptr;
    const auto readySlot = frame.FindSlot("ready");
    if (readySlot) {
        auto readySlotValue = readySlot->Value.AsString();
        NAlice::NData::NVoiceprint::EGender gender;
        if (!NAlice::NData::NVoiceprint::EGender_Parse(to_title(readySlotValue), &gender)) {
            LOG_WARN(logger) << "Failed to parse gender: " << (readySlotValue.Empty() ? "<none>" : readySlotValue);
            gender = NAlice::NData::NVoiceprint::EGender::Undefined;
        }

        EnrollState_.SetGender(RollbackGender(gender));
        EnrollState_.SetGenderMementoReady(gender);
        EnrollState_.SetCurrentStage(TVoiceprintEnrollState::Collect);

        auto persId = GeneratePersId();
        EnrollState_.SetPersId(persId);

        AddSlot(RenderSlots_, "phrases_count", 0);
        directiveFunc = &AddEnrollmentStartDirective;
    } else {
        LOG_ERROR(logger) << "Failed to find ready slot";
        AddSlot(RenderSlots_, NLU_SLOT_IS_SERVER_ERROR, true);
        renderTemplate = NLG_ENROLL_FINISH;
    }

    auto patchedFrame = frame;
    patchedFrame.SetName(TString(ENROLL_COLLECT_FRAME));
    RenderResponse(builder, patchedFrame, renderTemplate, "", directiveFunc, readySlot);
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    const auto userNameSlot = frame.FindSlot(NLU_SLOT_USER_NAME);
    Y_ENSURE(userNameSlot, TStringBuilder{} << frame.Name() << " frame is expected to have " << NLU_SLOT_USER_NAME << " slot");
    return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ false, userNameSlot);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    // TODO(klim-roma): Consider cancelling voiceprint scenario + redirecting to another scenario
    return CheckAndRepeat(frame, builder, /* isServerRepeat= */ true);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return CheckAndRepeat(frame, builder, /* isServerRepeat= */ false);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    const auto userNameSlot = frame.FindSlot(NLU_SLOT_USER_NAME);
    if (userNameSlot) {
        return HandleFrameWithUserName(frame, builder, /* isChangeNameRequest = */ true, userNameSlot);
    } else {
        return CheckAndRepeat(frame, builder, /* isServerRepeat= */ true);
    }
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnexpectedFrame(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentWaitReadyState::HandleFrameWithUserName(
    const TFrame& frame,
    TRunResponseBuilder& builder,
    bool isChangeNameRequest,
    const TPtrWrapper<TSlot>& userNameSlot
)
{
    if (!CheckPrerequisites(isChangeNameRequest) || !CheckSwear(userNameSlot)) {
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, false);
        return {};
    }

    EnrollState_.SetUserName(userNameSlot->Value.AsString());
    auto patchedFrame = frame;
    patchedFrame.SetName(TString(ENROLL_COLLECT_FRAME));
    RenderResponse(builder, patchedFrame, NLG_ENROLL_COLLECT, "", nullptr, false);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
