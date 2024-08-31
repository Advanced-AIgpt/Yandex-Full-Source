#include "prepare_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf SHOW_ROUTE_FRAME = "alice.navi.show_route";
constexpr TStringBuf SHOW_ROUTE_BASS_FRAME = "personal_assistant.scenarios.show_route";

constexpr TStringBuf WHAT_TO_SLOT = "what_to";
constexpr TStringBuf WHERE_TO_SLOT = "where_to";

TMaybe<TFrame> PrepareShowRouteFrame(const TFrame& rawFrame) {
    TFrame frame(TString{SHOW_ROUTE_BASS_FRAME});

    const auto whatToSource = rawFrame.FindSlot(WHAT_TO_SLOT);
    if (!whatToSource) {
        return Nothing();
    }
    TSlot whatToTarget(*whatToSource.Get());
    if (whatToTarget.Type == "custom.named_location") {
        whatToTarget.Type = "named_location";
    }
    whatToTarget.Value = TSlot::TValue{NNlu::TRequestNormalizer::Normalize(ELanguage::LANG_TUR, whatToTarget.Value.AsString())};
    frame.AddSlot(whatToTarget);

    const auto whereToSource = rawFrame.FindSlot(WHERE_TO_SLOT);
    if (whereToSource) {
        TSlot whereToTarget(*whereToSource.Get());
        if (whereToTarget.Type == "custom.special_location") {
            whereToTarget.Type = "special_location";
        }
        frame.AddSlot(whereToTarget);
    }

    return frame;
}

TMaybe<TFrame> ParseShowRouteFrame(const TScenarioInputWrapper& input) {
    const auto rawFrame = input.FindSemanticFrame(SHOW_ROUTE_FRAME);
    if (rawFrame) {
        const auto frame = TFrame::FromProto(*rawFrame);
        return PrepareShowRouteFrame(frame);
    }
    return Nothing();
}

} // namespace

void TShowRouteTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto maybeFrame = ParseShowRouteFrame(request.Input());
    if (!maybeFrame) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Failed to get alice.navi.show_route semantic frame";

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
