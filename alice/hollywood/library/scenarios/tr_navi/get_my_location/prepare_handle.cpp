#include "prepare_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

const TString GET_MY_LOCATION_FRAME = "personal_assistant.scenarios.get_my_location";

} // namespace

void TGetMyLocationTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto frameProto = request.Input().FindSemanticFrame(GET_MY_LOCATION_FRAME);
    if (!frameProto) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Failed to get get_my_location semantic frame";

        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TRunResponseBuilder responseBuilder(&nlgWrapper);
        responseBuilder.SetIrrelevant();
        responseBuilder.CreateResponseBodyBuilder();

        const auto response = *std::move(responseBuilder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        return;
    }

    const auto frame = TFrame::FromProto(*frameProto);

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
