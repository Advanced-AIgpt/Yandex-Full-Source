#include "render_handle.h"

#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

namespace NAlice::NHollywood::NTrNavi {

void TGeneralConversationTrRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto bassResponseBody = RetireBassRequest(ctx);
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder responseBuilder(&nlgWrapper);
    auto& bodyBuilder = responseBuilder.GetResponseBodyBuilder() ? *responseBuilder.GetResponseBodyBuilder()
                                                                   : responseBuilder.CreateResponseBodyBuilder();
    auto& logger = ctx.Ctx.Logger();

    const auto& formName = bassResponseBody["form"]["name"].GetString();
    if (formName != GENERAL_CONVERSATION_FRAME) {
        LOG_WARNING(logger) << "Bass answered with different form";

        responseBuilder.SetIrrelevant();
        responseBuilder.CreateResponseBodyBuilder();

        const auto response = std::move(responseBuilder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    const auto& slots = bassResponseBody["form"]["slots"].GetArray();
    const auto* reply = FindIfPtr(slots, [](const NJson::TJsonValue& slot) {return slot["name"].GetString() == "reply"; });
    if (!reply || (*reply)["value"].IsNull()) {
        PrepareResponseWithPhrase(bodyBuilder, logger, request, BEG_YOUR_PARDON);

        const auto response = std::move(responseBuilder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    TNlgData nlgData{ctx.Ctx.Logger(), request};
    nlgData.Context["text"] = (*reply)["value"];
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_GENERAL_CONVERSATION, "render_result", /* buttons = */ {}, nlgData);

    const auto response = std::move(responseBuilder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

}  // namespace NAlice::NHollywood::NTrNavi
