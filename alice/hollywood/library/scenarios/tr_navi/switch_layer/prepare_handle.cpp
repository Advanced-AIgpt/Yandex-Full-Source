#include "prepare_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf SWITCH_LAYER_FRAME = "alice.navi.switch_layer";
constexpr TStringBuf SHOW_LAYER_BASS_FRAME = "personal_assistant.navi.show_layer";
constexpr TStringBuf HIDE_LAYER_BASS_FRAME = "personal_assistant.navi.hide_layer";
constexpr TStringBuf ACTION_SLOT = "action";

TMaybe<TFrame> PrepareSwitchLayerFrame(const TFrame& rawFrame) {
    TStringBuf frameName;

    const auto actionSource = rawFrame.FindSlot(ACTION_SLOT);
    if (!actionSource) {
        return Nothing();
    } else if (actionSource->Value.AsString() == "show_layer") {
        frameName = SHOW_LAYER_BASS_FRAME;
    } else if (actionSource->Value.AsString() == "hide_layer") {
        frameName = HIDE_LAYER_BASS_FRAME;
    } else {
        return Nothing();
    }
    TFrame frame(TString{frameName});

    TSlot layer{
        "layer",
        "layer",
        TSlot::TValue("traffic")
    };
    frame.AddSlot(layer);

    return frame;
}

TMaybe<TFrame> ParseSwitchLayerFrame(const TScenarioInputWrapper& input) {
    const auto rawFrame = input.FindSemanticFrame(SWITCH_LAYER_FRAME);
    if (rawFrame) {
        const auto frame = TFrame::FromProto(*rawFrame);
        return PrepareSwitchLayerFrame(frame);
    }
    return Nothing();
}

} // namespace

void TSwitchLayerTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto maybeFrame = ParseSwitchLayerFrame(request.Input());
    if (!maybeFrame) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Failed to get " << SWITCH_LAYER_FRAME << " semantic frame";

        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TRunResponseBuilder responseBuilder(&nlgWrapper);
        responseBuilder.SetIrrelevant();
        responseBuilder.CreateResponseBodyBuilder();

        const auto response = *std::move(responseBuilder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        return;
    }
    const auto frame = *maybeFrame;
    const auto bassRequest = PrepareBassVinsRequest(
        ctx.Ctx.Logger(),
        request,
        frame,
        /* sourceTextProvider= */ nullptr,
        ctx.RequestMeta,
        /* imageSearch= */ false,
        ctx.AppHostParams
    );
    AddBassRequestItems(ctx, bassRequest);
}

}  // namespace NAlice::NHollywood::NTrNavi
