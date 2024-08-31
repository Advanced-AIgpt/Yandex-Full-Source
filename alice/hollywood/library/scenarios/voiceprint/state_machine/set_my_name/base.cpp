#include "set_my_name_run.h"

#include <alice/hollywood/library/scenarios/voiceprint/common.h>

#include <alice/hollywood/library/frame/frame.h>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

namespace {

TString UnexpectedFrameInStateMsg(const TString& frameName, const TString& stateName) {
    return TStringBuilder{} << "got " << frameName << " frame in " << stateName << " state";
}

} // namespace

TSetMyNameRunStateBase::TSetMyNameRunStateBase(TSetMyNameRunContext* context)
    : Context_{context}
    , SetMyNameState_(*Context_->ScenarioStateProto().MutableVoiceprintSetMyNameState())
{
}

TSetMyNameRunStateBase::TRunHandleResult TSetMyNameRunStateBase::HandleSetMyNameFrame(const TFrame& frame, TRunResponseBuilder& builder) {
    return ReportUnknownState(frame, builder);
}

TSetMyNameRunStateBase::TRunHandleResult TSetMyNameRunStateBase::Irrelevant() const {
    return {/* newState = */ nullptr, /* continueProcessing = */ false, /* isIrrelevant = */ true};
}

TSetMyNameRunStateBase::TRunHandleResult TSetMyNameRunStateBase::ReportUnknownState(const TFrame& frame, TRunResponseBuilder& builder) {
    NAlice::NHollywood::NVoiceprint::Irrelevant(Context_->Logger(), Context_->GetRequest(), builder, EIrrelevantType::UnknownStage,
                                                UnexpectedFrameInStateMsg(frame.Name(), Name()));
    ClearSetMyNameState();
    return {};
}

void TSetMyNameRunStateBase::RenderResponse(TRunResponseBuilder& builder, const TFrame& frame, bool isServerError) {
    auto& logger = Context_->Logger();
    const auto& request = Context_->GetRequest();

    auto renderForm = NJson::TJsonMap();
    renderForm["slots"] = RenderSlots_;

    TNlgData nlgData{logger, request};
    nlgData.Context["attentions"].SetType(NJson::JSON_MAP);
    nlgData.Form = renderForm;
    if (isServerError) {
        nlgData.AddAttention(ATTENTION_SERVER_ERROR);
    }

    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetProductScenarioName(SCENARIO_NAME);
    analyticsInfo.SetIntentName(frame.Name());

    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_SET_MY_NAME, "render_result", /* buttons = */ {}, nlgData);
}

void TSetMyNameRunStateBase::ClearSetMyNameState() {
    LOG_INFO(Context_->Logger()) << "Clearing set_my_name state";
    SetMyNameState_ = TVoiceprintSetMyNameState();
}

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
