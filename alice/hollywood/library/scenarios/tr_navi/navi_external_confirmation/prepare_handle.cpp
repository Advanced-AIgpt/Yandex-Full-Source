#include "prepare_handle.h"

#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf NAVI_EXTERNAL_CONFIRMATION_FRAME = "alice.vinsless.navi.external_confirmation";
constexpr TStringBuf BASS_FRAME = "personal_assistant.scenarios.show_route";
constexpr TStringBuf CONFIRMATION_SLOT = "confirmation";


TMaybe<TFrame> PrepareNaviExternalConfirmationFrame(const TFrame& rawFrame) {
    TFrame bassFrame(TString{BASS_FRAME});

    const auto confirmationSource = rawFrame.FindSlot(CONFIRMATION_SLOT);
    if (!confirmationSource) {
        return Nothing();
    }
    TSlot confirmationTarget(*confirmationSource.Get());
    confirmationTarget.Type = "confirmation";
    bassFrame.AddSlot(confirmationTarget);

    return bassFrame;
}

TMaybe<TFrame> ParseNaviExternalConfirmationFrame(const TScenarioInputWrapper& input) {
    const auto rawFrame = input.FindSemanticFrame(NAVI_EXTERNAL_CONFIRMATION_FRAME);
    if (rawFrame) {
        const auto frame = TFrame::FromProto(*rawFrame);
        return PrepareNaviExternalConfirmationFrame(frame);
    }
    return Nothing();
}

bool IsNaviWaitingForConfirmation(const TScenarioRunRequestWrapper& request) {
    if (request.HasExpFlag(EXP_NAVIGATOR_ALICE_CONFIRMATION))
        return true;
    const auto& deviceState = request.Proto().GetBaseRequest().GetDeviceState();
    if (deviceState.HasNavigator()) {
        for (const auto& state : deviceState.GetNavigator().GetStates()) {
            if (state == TStringBuf("waiting_for_route_confirmation")) {
                return true;
            }
        }
    }
    return false;
}

void SetIrrelevantResponse(TNlgWrapper& nlgWrapper, NAppHost::IServiceContext& serviceCtx) {
    TRunResponseBuilder responseBuilder(&nlgWrapper);
    responseBuilder.SetIrrelevant();
    responseBuilder.CreateResponseBodyBuilder();

    const auto response = *std::move(responseBuilder).BuildResponse();
    serviceCtx.AddProtobufItem(response, RESPONSE_ITEM);
}

} // namespace

void TNaviExternalConfirmationTrPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    if (!IsNaviWaitingForConfirmation(request)) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "Navi is not waiting for confirmation";
        auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        SetIrrelevantResponse(nlgWrapper, ctx.ServiceCtx);
        return;
    }

    const auto maybeFrame = ParseNaviExternalConfirmationFrame(request.Input());
    if (!maybeFrame) {
        LOG_DEBUG(ctx.Ctx.Logger()) << "NaviIsWaitingForConfirmation navi_external_confirmation not recognized";
        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TRunResponseBuilder responseBuilder(&nlgWrapper);
        auto& responseBodyBuilder = responseBuilder.CreateResponseBodyBuilder();
        TNlgData nlgData{ctx.Ctx.Logger(), request};

        responseBodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_NAVI_EXTERNAL_CONFIRMATION, "render_result", /* buttons = */ {}, nlgData);
        responseBodyBuilder.SetShouldListen(true);

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
