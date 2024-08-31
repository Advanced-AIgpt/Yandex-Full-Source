#include "enroll_run.h"

#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/scenarios/voiceprint/common.h>
#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/server_biometry.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/protos/data/scenario/voiceprint/personalization_data.pb.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

void AddEnrollmentCancelDirective(TRTLogger& logger, const TVoiceprintEnrollState& enrollState, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TDirective directive;
    auto& enrollCancelDirective = *directive.MutableEnrollmentCancelDirective();
    enrollCancelDirective.SetPersId(enrollState.GetPersId());
    LOG_INFO(logger) << "AddEnrollmentCancelDirective: " << directive;
    bodyBuilder.AddDirective(std::move(directive));
    bodyBuilder.AddTtsPlayPlaceholderDirective();
}

TString UnexpectedFrameInStageMsg(const TString& frameName, TVoiceprintEnrollState::EStage stage) {
    return TStringBuilder{} << "got " << frameName << " frame on " << TVoiceprintEnrollState::EStage_Name(stage) << " stage";
}

bool IsFemale(const TString& genderString) {
    return genderString == to_lower(NAlice::NData::NVoiceprint::EGender_Name(NAlice::NData::NVoiceprint::EGender::Female));
}

} // namespace

TEnrollmentRunStateBase::TEnrollmentRunStateBase(TEnrollmentRunContext* context)
    : Context_{context}
    , EnrollState_(*Context_->ScenarioStateProto().MutableVoiceprintEnrollState())
    , NlgData_{Context_->Logger(), Context_->GetRequest()}
{
    UpdateVoiceprintEnrollState(EnrollState_);
    NlgData_.Context["attentions"].SetType(NJson::JSON_MAP);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleEnrollNewGuestFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleEnrollFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleEnrollCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleEnrollReadyFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleEnrollStartFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleEnrollCollectFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleRepeatFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::HandleGuestEnrollmentFinishFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::ReportUnexpectedFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    Irrelevant(Context_->Logger(), Context_->GetRequest(), builder, EIrrelevantType::UnexpectedFrame,
               UnexpectedFrameInStageMsg(frame.Name(), EnrollState_.GetCurrentStage()));
    return {};
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::ReportUnknownStage(const TFrame& frame, TRunResponseBuilder& builder) {
    Irrelevant(Context_->Logger(), Context_->GetRequest(), builder, EIrrelevantType::UnknownStage,
               UnexpectedFrameInStageMsg(frame.Name(), EnrollState_.GetCurrentStage()));
    return {};
}

void TEnrollmentRunStateBase::RenderResponse(
    TRunResponseBuilder& builder,
    const TFrame& frame,
    TStringBuf renderTemplate,
    const TString& extraDirective,
    EnrollmentDirectiveFunc directiveFunc,
    bool isReady
)
{
    auto& logger = Context_->Logger();
    auto& request = Context_->GetRequest();
    auto& scState = Context_->ScenarioStateProto();

    const auto userName = EnrollState_.GetUserName();
    if (userName) {
        AddSlot(RenderSlots_, "user_name", userName);
        AddSlot(RenderSlots_, "user_name_frozen", userName);
    }

    AddSlot(RenderSlots_, "ready", isReady);
    AddSlot(RenderSlots_, "ready_frozen", isReady);

    if (EnrollState_.GetIsBioCapabilitySupported() && request.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_MULTIACCOUNT)) {
        AddSlot(RenderSlots_, SLOT_IS_MULTIACCOUNT_ENABLED, true);
    }

    auto renderForm = NJson::TJsonMap();
    renderForm["slots"] = RenderSlots_;
    NlgData_.Form = renderForm;

    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetProductScenarioName(SCENARIO_NAME);
    analyticsInfo.SetIntentName(frame.Name());

    Y_ENSURE(renderTemplate);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(renderTemplate, "render_result", /* buttons = */ {}, NlgData_);
    if (extraDirective) {
        bodyBuilder.AddClientActionDirective(extraDirective, TStringBuilder() << "voiceprint_" << extraDirective, {});
    }
    if (directiveFunc) {
        if (request.HasExpFlag(EXP_HW_VOICEPRINT_ENROLLMENT_DIRECTIVES)) {
            directiveFunc(logger, EnrollState_, bodyBuilder);
        }
    }

    if (renderTemplate == NLG_ENROLL_FINISH) {
        ClearEnrollState();
    } else {
        bodyBuilder.SetShouldListen(true);
        bodyBuilder.SetExpectsRequest(true);
    }
    bodyBuilder.SetState(scState);
}

bool TEnrollmentRunStateBase::CheckSwear(const TPtrWrapper<TSlot>& userNameSlot) {
    if (userNameSlot->Type != SWEAR_SLOT_TYPE) {
        return true;
    }

    LOG_INFO(Context_->Logger()) << "user_name slot is swear. Rendering corresponding response...";
    AddSlot(RenderSlots_, SLOT_SWEAR_USER_NAME, true);
    return false;
}

bool TEnrollmentRunStateBase::CheckPrerequisites(bool isChangeNameRequest, bool isNewGuestFrame) {
    auto& logger = Context_->Logger();
    auto& request = Context_->GetRequest();
    auto isBioCapabilitySupported = EnrollState_.GetIsBioCapabilitySupported();

    if (!isBioCapabilitySupported && !Context_->IsValidRegion()) {
        LOG_INFO(logger) << "Invalid region is detected. Enrollment won't be continued";
        NlgData_.AddAttention(ATTENTION_INVALID_REGION);
        return false;
    }

    const auto& uid = EnrollState_.GetUid();
    if (!uid) {
        LOG_INFO(logger) << "Empty uid in scenario state found. Exiting...";
        AddSlot(RenderSlots_, "is_need_login", true);
        return false;
    }

    AddSlot(RenderSlots_, "is_change_name", isChangeNameRequest);

    if (isNewGuestFrame) {
        return true;
    }

    auto isKnownUser = false;
    if (isBioCapabilitySupported) {
        LOG_INFO(logger) << "Processing client biometry...";
        auto guestOptions = GetGuestOptionsProto(request);
        if (!guestOptions) {
            LOG_ERROR(logger) << "No TGuestOptions data source were found";
            AddSlot(RenderSlots_, NLU_SLOT_IS_SERVER_ERROR, true);
            return false;
        }
        
        auto isKnownUser = HasMatch(guestOptions);
        if (isKnownUser) {
            LOG_INFO(logger) << "Known user is detected. Got TGuestOptions data source with YandexUID=" << guestOptions->GetYandexUID();
            NlgData_.AddAttention(ATTENTION_KNOWN_USER);
        }

        // TODO(klim-roma): add push-sending logic and change nlg according to https://st.yandex-team.ru/ALICEPRODUCT-427
        if (guestOptions->GetIsOwnerEnrolled() &&
            EnrollState_.GetCurrentStage() == TVoiceprintEnrollState::NotStarted)
        {
            LOG_INFO(logger) << "Owner is enrolled already. Guests are expected to be enrolled via TSF. Exiting...";
            if (request.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_MULTIACCOUNT)) {
                auto ownerName = GetOwnerNameFromDataSync(request);
                if (ownerName) {
                    AddSlot(RenderSlots_, "owner_name", *ownerName);
                } else {
                    LOG_ERROR(logger) << "Failed to get owner's name from DataSync";
                    AddSlot(RenderSlots_, NLU_SLOT_IS_SERVER_ERROR, true);
                }

                auto gender = GetGenderFromDataSync(request);
                if (gender) {
                    AddSlot(RenderSlots_, "is_female", IsFemale(*gender));
                } else {
                    LOG_ERROR(logger) << "Failed to get owner's gender from DataSync";
                    AddSlot(RenderSlots_, NLU_SLOT_IS_SERVER_ERROR, true);
                }

                AddSlot(RenderSlots_, "enrollment_via_iot_is_needed", true);
            } else {
                AddSlot(RenderSlots_, NLU_SLOT_IS_TOO_MANY_ENROLLED_USERS, true);
            }
            return false;
        }
    } else {
        LOG_INFO(logger) << "Processing server biometry...";
        auto classicBiometry = ProcessServerBiometry(logger, request, uid,
                                                       NBiometry::TBiometry::EMode::HighTPR,
                                                       /* addMaxAccuracyIncognitoCheck = */ false);
        
        auto isKnownUser = classicBiometry && !classicBiometry->IsIncognitoUser;
        if (isKnownUser) {
            LOG_INFO(logger) << "Known user is detected";
            NlgData_.AddAttention(ATTENTION_KNOWN_USER);
        }

        if (classicBiometry && EnrollState_.GetCurrentStage() == TVoiceprintEnrollState::NotStarted) {
            LOG_INFO(logger) << "Multiple voice enrollments in server biometry are not supported. Exiting...";
            AddSlot(RenderSlots_, NLU_SLOT_IS_TOO_MANY_ENROLLED_USERS, true);
            return false;
        }
    }

    if (isKnownUser) {
        return false;
    }

    return true;
}

void TEnrollmentRunStateBase::CancelActiveEnrollment(const TFrame& frame, TRunResponseBuilder& builder) {
    LOG_INFO(Context_->Logger()) << "Canceling enrollment...";
    EnrollmentDirectiveFunc directiveFunc = nullptr;
    if (EnrollState_.GetPersId()) {
        directiveFunc = &AddEnrollmentCancelDirective;
    }

    RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", directiveFunc, false);
}

void TEnrollmentRunStateBase::RenderRepeat(const TFrame& frame, TRunResponseBuilder& builder,
                                           TStringBuf forceFrameName, TStringBuf renderTemplate,
                                           bool isReady) {
    auto patchedFrame = frame;
    patchedFrame.SetName(TString(forceFrameName));
    RenderResponse(builder, patchedFrame, renderTemplate, "", nullptr, isReady);
}

void TEnrollmentRunStateBase::ClearEnrollState() {
    LOG_INFO(Context_->Logger()) << "Clearing enrollment state";
    EnrollState_ = TVoiceprintEnrollState();
}

TEnrollmentRunStateBase::TRunHandleResult TEnrollmentRunStateBase::CheckAndRepeat(
    const TFrame& frame, TRunResponseBuilder& builder, bool isServerRepeat
)
{
    if (!CheckPrerequisites()) {
        RenderResponse(builder, frame, NLG_ENROLL_FINISH, "", nullptr, false);
        return {};
    }
    
    AddSlot(RenderSlots_, "is_server_repeat", isServerRepeat);
    RenderRepeat(frame, builder, ENROLL_COLLECT_FRAME, NLG_ENROLL_COLLECT, false);
    return {};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
