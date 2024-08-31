#include "main.h"

#include <alice/hollywood/library/scenarios/voiceprint/proto/voiceprint_arguments.pb.h>
#include <alice/hollywood/library/scenarios/voiceprint/util/util.h>

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/proto/proto.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NVoiceprint {

void TApplyRenderHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper requestWrapper{requestProto, ctx.ServiceCtx};
    const auto& applyArgs = requestWrapper.UnpackArguments<TVoiceprintArguments>();
    auto enrollState = applyArgs.GetVoiceprintEnrollState();
    UpdateVoiceprintEnrollState(enrollState);

    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), requestWrapper, ctx.Rng, ctx.UserLang);
    auto bassResponse = RetireBassRequest(ctx);

    if (!bassResponse.Has("blocks")) {
        bassResponse["blocks"] = NJson::TJsonArray();
    }

    TApplyResponseBuilder response(&nlg);

    auto isMultiaccEnabled = enrollState.GetIsBioCapabilitySupported() && requestWrapper.HasExpFlag(EXP_HW_VOICEPRINT_ENABLE_MULTIACCOUNT);

    TStringBuf renderTemplate;
    if (isMultiaccEnabled) {
        renderTemplate = NLG_ENROLL_MULTIACC_FINISH_SUCCESS;
    } else {
        renderTemplate = NLG_ENROLL_FINISH;
    }

    const auto suggestAutoAction = false;
    TBassResponseRenderer bassRenderer(requestWrapper, requestWrapper.Input(), response, logger, suggestAutoAction);
    bassRenderer.Render(renderTemplate, "render_result", bassResponse, TString(ENROLL_FINISH_FRAME));

    auto& bodyBuilder = *response.GetResponseBodyBuilder();

    if (requestWrapper.HasExpFlag(EXP_HW_VOICEPRINT_ENROLLMENT_DIRECTIVES)) {
        auto applyArgs = requestWrapper.UnpackArguments<TVoiceprintArguments>();
        Y_ENSURE(applyArgs.HasVoiceprintEnrollState(), "Only enrollment case in apply_render stage is expected");
        const auto& enrollState = applyArgs.GetVoiceprintEnrollState();
        AddEnrollmentFinishDirective(logger, enrollState, bodyBuilder, /* sendGuestEnrollmentFinishFrame = */ false);
    }

    bodyBuilder.SetShouldListen(!isMultiaccEnabled);

    const auto renderedAns = *std::move(response).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(renderedAns, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NVoiceprint
