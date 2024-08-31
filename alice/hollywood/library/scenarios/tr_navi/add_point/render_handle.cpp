#include "render_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/metrics/metrics.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

namespace NAlice::NHollywood::NTrNavi {

namespace {

constexpr TStringBuf TEMPLATE_ADD_POINT = "add_point";

} // namespace

void TAddPointTrRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto bassResponseBody = RetireBassRequest(ctx);
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);

    TRunResponseBuilder builder(&nlgWrapper);
    TBassResponseRenderer bassRenderer(
        request,
        request.Input(),
        builder,
        ctx.Ctx.Logger(),
        /* suggestAutoAction= */ false
    );
    bassRenderer.Render(TEMPLATE_ADD_POINT, "render_result", bassResponseBody);

    const auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

}  // namespace NAlice::NHollywood::NTrNavi
