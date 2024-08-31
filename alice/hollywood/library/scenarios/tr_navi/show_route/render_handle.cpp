#include "render_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf TEMPLATE_SHOW_ROUTE = "show_route";
constexpr TStringBuf REMEMBER_NAMED_LOCATION_BASS_FRAME = "personal_assistant.scenarios.remember_named_location";

constexpr TStringBuf NLG_PHRASE_ROUTE = "render_route";
constexpr TStringBuf NLG_PHRASE_SEARCH = "render_search";
constexpr TStringBuf NLG_PHRASE_REMEBER_NAMED_LOCATION = "remember_named_location";

TStringBuf GetNlgPhrase(const NJson::TJsonValue& bassResponseBody) {
    if (!bassResponseBody.Has("blocks")) {
        return {};
    }
    for (const auto& block : bassResponseBody["blocks"].GetArray()) {
        const NJson::TJsonValue* typePtr = nullptr;
        Y_ENSURE(block.GetValuePointer("type", &typePtr), "block must have `type` field");

        if (typePtr->GetStringSafe() == "command") {
            const NJson::TJsonValue* commandSubtypePtr = nullptr;
            Y_ENSURE(
                block.GetValuePointer("command_sub_type", &commandSubtypePtr),
                "command block must have `command_sub_type` field"
            );
            if (commandSubtypePtr->GetStringSafe() == "navi_build_route_on_map") {
                return NLG_PHRASE_ROUTE;
            } else {
                return NLG_PHRASE_SEARCH;
            }
        }
    }
    return {};
}

} // namespace

void TShowRouteTrRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto bassResponseBody = RetireBassRequest(ctx);
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    if (bassResponseBody["form"]["name"].GetString() == REMEMBER_NAMED_LOCATION_BASS_FRAME) {
        auto& bodyBuilder = builder.CreateResponseBodyBuilder();
        TNlgData nlgData{ctx.Ctx.Logger(), request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_SHOW_ROUTE, NLG_PHRASE_REMEBER_NAMED_LOCATION, /* buttons = */ {}, nlgData);
    } else {
        for (const auto& slot : bassResponseBody["form"]["slots"].GetArray()) {
            if (slot["name"] == "resolved_location_to" && !slot["value"].IsNull()) {
                TBassResponseRenderer bassRenderer(
                    request,
                    request.Input(),
                    builder,
                    ctx.Ctx.Logger(),
                    /* suggestAutoAction= */ false
                );
                bassRenderer.Render(TEMPLATE_SHOW_ROUTE, GetNlgPhrase(bassResponseBody), bassResponseBody);
                const auto response = std::move(builder).BuildResponse();
                ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
                return;
            }
        }
        LOG_WARNING(ctx.Ctx.Logger()) << "No route found";
        builder.SetIrrelevant();
        builder.CreateResponseBodyBuilder();
    }

    const auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

}  // namespace NAlice::NHollywood::NTrNavi
