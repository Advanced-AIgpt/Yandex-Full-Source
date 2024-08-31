#include "repeat_after_me.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NRepeatAfterMe {

namespace {

constexpr TStringBuf REPEAT_AFTER_ME_FRAME_NAME = "alice.repeat_after_me";

constexpr TStringBuf TEXT_SLOT = "text";
constexpr TStringBuf VOICE_SLOT = "voice";

std::pair<TString, TString> ConstructTextAndVoice(const TFrame& frame) {
    const auto textSlot = frame.FindSlot(TEXT_SLOT);
    const auto voiceSlot = frame.FindSlot(VOICE_SLOT);

    const TStringBuf text = textSlot ? textSlot->Value.AsString() : "";
    const TStringBuf voice = voiceSlot ? voiceSlot->Value.AsString() : text;

    return std::make_pair(TString{text}, TString{voice});
}

TFrame ConstructFrame(const TScenarioRunRequestWrapper& request) {
    const auto frameProto = request.Input().FindSemanticFrame(REPEAT_AFTER_ME_FRAME_NAME);
    Y_ENSURE(frameProto);
    return TFrame::FromProto(*frameProto);
}

std::unique_ptr<NScenarios::TScenarioRunResponse>
ConstructResponse(TRTLogger& logger, const TScenarioRunRequestWrapper& request) {
    const auto frame = ConstructFrame(request);
    auto [text, voice] = ConstructTextAndVoice(frame);
    LOG_INFO(logger) << "Repeat text \"" << text << "\", voice \"" << voice << "\"";

    TRunResponseBuilder builder;
    TResponseBodyBuilder& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    bodyBuilder.AddRawTextWithButtonsAndVoice(/* text = */ text, /* voice = */ voice, /* buttons = */ {});

    return std::move(builder).BuildResponse();
}

} // anonymous namespace

void TRepeatAfterMeRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto response = ConstructResponse(ctx.Ctx.Logger(), request);
    ctx.ServiceCtx.AddProtobufItem(std::move(*response), RESPONSE_ITEM);
}

REGISTER_SCENARIO("repeat_after_me",
                  AddHandle<TRepeatAfterMeRunHandle>());

}  // namespace NAlice::NHollywood::NRepeatAfterMe
