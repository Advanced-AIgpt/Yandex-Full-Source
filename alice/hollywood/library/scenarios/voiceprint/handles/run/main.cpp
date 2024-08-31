#include "impl.h"
#include "main.h"

#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

namespace NAlice::NHollywood::NVoiceprint {

namespace NImpl {

void TRunHandleImpl::Do() {
    if (!CheckVoiceprintSupported()) {
        return;
    }

    LogInfoScenarioState();
    auto ctx = VoiceprintCtx_.Ctx;

    auto isEnrollmentSuggested = VoiceprintCtx_.ScenarioStateProto.GetIsEnrollmentSuggested();
    VoiceprintCtx_.ScenarioStateProto.ClearIsEnrollmentSuggested();

    // NOTE: the order of handles matters!!!

    if (IsCollectingPhrases(VoiceprintCtx_.ScenarioStateProto)) {
        auto resp = HandleEnroll(isEnrollmentSuggested);
        Y_ENSURE(resp, "In Collect state any frame is expected to be successfully handled by HandleEnroll method");
        return ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
    }

    if (auto resp = HandleRemove()) {
        return ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
    }

    if (auto resp = HandleWhatIsMyName()) {
        return ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
    }

    if (auto resp = HandleSetMyName()) {
        return ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
    }

    if (auto resp = HandleEnroll(isEnrollmentSuggested)) {
        return ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
    }
    
    if (isEnrollmentSuggested) {
        return RenderDeclineResponse();
    }

    IrrelevantResponse(EIrrelevantType::UnsupportedFeature, "no scenario case has relevant answer");
}

} // namespace NImpl

void TRunHandle::Do(TScenarioHandleContext& ctx) const {
    NImpl::TRunHandleImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NVoiceprint
