#include "remove_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

TString UnexpectedFrameInStageMsg(const TString& frameName, TVoiceprintRemoveState::EStage stage) {
    return TStringBuilder{} << "got " << frameName << " frame on " << TVoiceprintRemoveState::EStage_Name(stage) << " stage";
}

} // namespace

TRemoveRunStateBase::TRemoveRunStateBase(TRemoveRunContext* context)
    : Context_{context}
    , RemoveState_(*Context_->ScenarioStateProto().MutableVoiceprintRemoveState())
    , NlgData_{Context_->Logger(), Context_->GetRequest()}
{
    NlgData_.Context["attentions"].SetType(NJson::JSON_MAP);
}

TRemoveRunStateBase::TRunHandleResult TRemoveRunStateBase::HandleRemoveFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TRemoveRunStateBase::TRunHandleResult TRemoveRunStateBase::HandleCancelFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownStage(frame, builder);
}

TRemoveRunStateBase::TRunHandleResult TRemoveRunStateBase::ReportUnexpectedFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    Irrelevant(Context_->Logger(), Context_->GetRequest(), builder, EIrrelevantType::UnexpectedFrame,
               UnexpectedFrameInStageMsg(frame.Name(), RemoveState_.GetCurrentStage()));
    return {};
}

TRemoveRunStateBase::TRunHandleResult TRemoveRunStateBase::ReportUnknownStage(const TFrame& frame, TRunResponseBuilder& builder) {
    Irrelevant(Context_->Logger(), Context_->GetRequest(), builder, EIrrelevantType::UnknownStage,
               UnexpectedFrameInStageMsg(frame.Name(), RemoveState_.GetCurrentStage()));
    return {};
}

void TRemoveRunStateBase::RenderResponse(
    TRunResponseBuilder& builder,
    const TFrame& frame,
    TStringBuf renderTemplate,
    bool expectsAnswer
)
{
    auto& scState = Context_->ScenarioStateProto();

    LOG_DEBUG(Context_->Logger()) << RemoveState_;

    const auto userName = RemoveState_.GetUserName();
    if (!userName.Empty()) {
        AddSlot(RenderSlots_, "user_name", userName);
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

    if (renderTemplate != NLG_REMOVE_CONFIRM) {
        ClearRemoveState();
    }
    if (expectsAnswer && renderTemplate == NLG_REMOVE_UNKNOWN_USER) {
        TFrameNluHint nluHint;
        nluHint.SetFrameName(TString(CONFIRM_FRAME));
        bodyBuilder.AddNluHint(std::move(nluHint));
    }
    bodyBuilder.SetShouldListen(expectsAnswer);
    bodyBuilder.SetExpectsRequest(expectsAnswer);
    bodyBuilder.SetState(scState);
}

void TRemoveRunStateBase::ClearRemoveState() {
    LOG_INFO(Context_->Logger()) << "Clearing remove state...";
    RemoveState_ = TVoiceprintRemoveState{};
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
