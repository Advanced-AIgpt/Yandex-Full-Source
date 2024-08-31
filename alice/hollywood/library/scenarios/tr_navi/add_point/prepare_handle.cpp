#include "prepare_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf ADD_POINT_FRAME = "personal_assistant.navi.add_point";
constexpr TStringBuf ROAD_EVENT_SLOT = "road_event";
constexpr TStringBuf LANE_SLOT = "lane";
constexpr TStringBuf COMMENT_SLOT = "comment";

TMaybe<TFrame> PrepareAddPointFrame(const TFrame& rawFrame, const TString& utterance) {
    TFrame frame(TString{ADD_POINT_FRAME});

    const auto roadEventSource = rawFrame.FindSlot(ROAD_EVENT_SLOT);
    if (roadEventSource) {
        TSlot roadEventTarget(*roadEventSource.Get());
        roadEventTarget.Type = "road_event";
        frame.AddSlot(roadEventTarget);
    }

    const auto laneSource = rawFrame.FindSlot(LANE_SLOT);
    TSlot laneTarget;
    if (laneSource) {
        laneTarget = TSlot(*laneSource.Get());
    } else {
        laneTarget.Name = "lane";
        laneTarget.Value = TSlot::TValue{""};
    }
    laneTarget.Type = "string";
    frame.AddSlot(laneTarget);

    TSlot comment{
        TString{COMMENT_SLOT},
        "string",
        TSlot::TValue(utterance)
    };
    frame.AddSlot(comment);

    return frame;
}

TMaybe<TFrame> ParseAddPointFrame(const TScenarioInputWrapper& input) {
    const auto rawFrame = input.FindSemanticFrame(ADD_POINT_FRAME);
    if (rawFrame) {
        const auto frame = TFrame::FromProto(*rawFrame);
        return PrepareAddPointFrame(frame, input.Utterance());
    }
    return Nothing();
}

} // namespace

void TAddPointTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto maybeFrame = ParseAddPointFrame(request.Input());
    if (!maybeFrame) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Failed to get " << ADD_POINT_FRAME << " semantic frame";

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
