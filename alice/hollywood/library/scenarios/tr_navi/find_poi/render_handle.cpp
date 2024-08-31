#include "render_handle.h"

#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf TEMPLATE_FIND_POI = "find_poi";
constexpr TStringBuf LAST_FOUND_POI_SLOT = "last_found_poi";

} // namespace

void TFindPoiTrRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto bassResponseBody = RetireBassRequest(ctx);
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    const auto& formName = bassResponseBody["form"]["name"].GetString();
    if (formName != FIND_POI_BASS_FRAME) {
        LOG_WARNING(ctx.Ctx.Logger()) << "Bass answered with different form";

        builder.SetIrrelevant();
        builder.CreateResponseBodyBuilder();
        const auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    const auto& slots = bassResponseBody["form"]["slots"].GetArray();
    const auto* lastFoundPoi = FindIfPtr(slots, [](const NJson::TJsonValue& slot) {return slot["name"].GetString() == LAST_FOUND_POI_SLOT; });
    if (!lastFoundPoi || (*lastFoundPoi)["value"].IsNull()) {
        // TODO(ardulat): add usage of nothing_found nlg phrase
        LOG_WARNING(ctx.Ctx.Logger()) << "No poi found";

        builder.SetIrrelevant();
        builder.CreateResponseBodyBuilder();
        const auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        return;
    }

    TBassResponseRenderer bassRenderer(
        request,
        request.Input(),
        builder,
        ctx.Ctx.Logger(),
        /* suggestAutoAction= */ false
    );
    bassRenderer.Render(TEMPLATE_FIND_POI, "render_result", bassResponseBody);

    const auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

}  // namespace NAlice::NHollywood::NTrNavi
