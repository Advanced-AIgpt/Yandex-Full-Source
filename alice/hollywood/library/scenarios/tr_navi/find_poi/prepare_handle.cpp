#include "prepare_handle.h"

#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf WHAT_SLOT = "what";
constexpr TStringBuf FIND_POI_FRAME = "alice.navi.find_poi";

NScenarios::TScenarioRunResponse CreateIrrelevantResponse(TNlgWrapper& nlgWrapper) {
    TRunResponseBuilder responseBuilder(&nlgWrapper);

    responseBuilder.SetIrrelevant();
    responseBuilder.CreateResponseBodyBuilder();

    return *std::move(responseBuilder).BuildResponse();
}

TMaybe<TFrame> PrepareFindPoiBassFrame(const TFrame& rawFrame, const TStringBuf name) {
    TFrame frame(TString{name});
    for (auto slot : rawFrame.Slots()) {
        if (slot.Type == "custom.special_location") {
            slot.Type = "special_location";
        }
        if (slot.Name == WHAT_SLOT) {
            slot.Value = TSlot::TValue{NNlu::TRequestNormalizer::Normalize(ELanguage::LANG_TUR, slot.Value.AsString())};
        }
        frame.AddSlot(slot);
    }
    if (!rawFrame.FindSlot(WHAT_SLOT)) {
        frame.AddSlot(TSlot{TString(WHAT_SLOT),
                            "string",
                            TSlot::TValue("")});
    }
    return frame;
}

TMaybe<TFrame> ParseFindPoiFrame(const TScenarioInputWrapper& input) {
    const auto rawFrame = input.FindSemanticFrame(FIND_POI_FRAME);
    if (rawFrame) {
        const auto frame = TFrame::FromProto(*rawFrame);
        return PrepareFindPoiBassFrame(frame, FIND_POI_BASS_FRAME);
    }
    return Nothing();
}

} // namespace

void TFindPoiTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);

    const auto maybeFrame = ParseFindPoiFrame(request.Input());
    if (!maybeFrame) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Failed to get " << FIND_POI_FRAME << " semantic frame";

        const auto response = CreateIrrelevantResponse(nlgWrapper);
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        return;
    }

    const auto frame = *maybeFrame;
    const auto slot = frame.FindSlot(WHAT_SLOT);
    if (!slot) {
        LOG_WARNING(ctx.Ctx.Logger()) << "No poi detected";

        const auto response = CreateIrrelevantResponse(nlgWrapper);
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        return;
    }

    const auto bassRequest = PrepareBassVinsRequest(
        ctx.Ctx.Logger(),
        request,
        frame,
        /* textFrame= */ nullptr,
        ctx.RequestMeta,
        /* imageSearch= */ false,
        ctx.AppHostParams
    );
    AddBassRequestItems(ctx, bassRequest);
}

}  // namespace NAlice::NHollywood::NTrNavi
