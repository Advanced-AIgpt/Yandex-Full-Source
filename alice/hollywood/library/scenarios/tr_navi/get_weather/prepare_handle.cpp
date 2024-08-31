#include "prepare_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf GET_WEATHER_FRAME = "personal_assistant.scenarios.get_weather";
constexpr TStringBuf WHEN_SLOT = "when";

TMaybe<TFrame> PrepareGetWeatherFrame(const TFrame& rawFrame) {
    TFrame frame(TString{GET_WEATHER_FRAME});
    const auto whenSource = rawFrame.FindSlot(WHEN_SLOT);
    if (!whenSource) {
        return frame;
    }
    TSlot whenTarget(*whenSource.Get());
    if (whenTarget.Type == "fst.datetime") {
        whenTarget.Type = "datetime";
    } else if (whenTarget.Type == "fst.datetime_range") {
        whenTarget.Type = "datetime_range";
    } else {
        return Nothing();
    }
    frame.AddSlot(whenTarget);

    return frame;
}

TMaybe<TFrame> ParseGetWeatherFrame(const TScenarioInputWrapper& input) {
    const auto rawFrame = input.FindSemanticFrame(GET_WEATHER_FRAME);
    if (rawFrame) {
        const auto frame = TFrame::FromProto(*rawFrame);
        return PrepareGetWeatherFrame(frame);
    }
    return Nothing();
}

} // namespace

void TGetWeatherTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto maybeFrame = ParseGetWeatherFrame(request.Input());
    if (!maybeFrame) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Failed to get personal_assistant.scenarios.get_weather semantic frame";

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
