#include "prepare_with_bass.h"

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/hollywood/library/scenarios/show_traffic_bass/names.h>
#include <alice/hollywood/library/scenarios/show_traffic_bass/proto/show_traffic_bass.pb.h>

#include "renderer.h"

namespace NAlice::NHollywood {

namespace {

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse> BassShowTrafficMainScreenPrepareDoImpl(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    const NScenarios::TRequestMeta& meta)
{
    TFrame frame{TString{NShowTrafficBass::CENTAUR_COLLECT_MAIN_SCREEN_TRAFFIC_SEMANTIC_FRAME}};
    return PrepareBassVinsRequest(
        ctx.Ctx.Logger(),
        request,
        frame,
        /* sourceTextProvider= */ nullptr,
        meta,
        /* imageSearch= */ false,
        ctx.AppHostParams
    );
}

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse> BassShowTrafficPrepareDoImpl(
    TScenarioHandleContext& ctx,
    TNlgWrapper& nlg,
    const TScenarioRunRequestWrapper& request,
    const NScenarios::TRequestMeta& meta)
{
    if (auto* callback = request.Input().GetCallback()) {
        NShowTrafficBass::TRenderer renderer{ctx, request};
        renderer.RenderFeedbackAnswer(callback);
        return *std::move(renderer.GetBuilder()).BuildResponse();
    }

    const auto detailsFrame = request.Input().FindSemanticFrame(NShowTrafficBass::SHOW_TRAFFIC_DETAILS_FRAME);

    const auto& rawState = request.BaseRequestProto().GetState();

    if (rawState.Is<TTrafficInfo>() && detailsFrame && !request.IsNewSession()) {
        TTrafficInfo trafficInfo;
        rawState.UnpackTo(&trafficInfo);
        auto frame = TFrame::FromProto(*detailsFrame);
        NShowTrafficBass::TRenderer renderer{ctx, request, &frame};
        renderer.GetMutableState()->PackFrom(trafficInfo);
        renderer.Render(NShowTrafficBass::SHOW_TRAFFIC_BASS_DETAILS_NLG);
        renderer.AddOpenUriDirective(trafficInfo.GetUrl(), NNaviDirectives::NAVI_SHOW_POINT_ON_MAP);
        renderer.AddOpenUriDirective(NNaviDirectives::SHOW_TRAFFIC_LAYER, NNaviDirectives::NAVI_LAYER_TRAFFIC);
        return *std::move(renderer.GetBuilder()).BuildResponse();
    }

    if(request.Input().FindSemanticFrame(NShowTrafficBass::COLLECT_MAIN_SCREEN_FRAME) || request.Input().FindSemanticFrame(NShowTrafficBass::COLLECT_WIDGET_GALLERY_FRAME)) {
        return BassShowTrafficMainScreenPrepareDoImpl(ctx, request, meta);
    }

    const auto mainFrame = request.Input().FindSemanticFrame(NShowTrafficBass::SHOW_TRAFFIC_FRAME);
    const auto ellipsisFrame = request.Input().FindSemanticFrame(NShowTrafficBass::SHOW_TRAFFIC_ELLIPSIS_FRAME);
    TPtrWrapper<TSemanticFrame> frameProto(nullptr, NShowTrafficBass::SHOW_TRAFFIC_FRAME);

    if (ellipsisFrame && !request.IsNewSession()) {
        frameProto = ellipsisFrame;
    } else {
        frameProto = mainFrame;
    }

    if (request.HasExpFlag(NShowTrafficBass::DISABLE_VOICE_FRAMES) || !frameProto) {
        TRunResponseBuilder response(&nlg);
        response.SetIrrelevant();
        return *std::move(response).BuildResponse();
    }

    const auto frame = TFrame::FromProto(*frameProto);

    return PrepareBassVinsRequest(
        ctx.Ctx.Logger(),
        request,
        frame,
        /* sourceTextProvider= */ nullptr,
        meta,
        /* imageSearch= */ false,
        ctx.AppHostParams
    );
}

} // namespace

void TBassShowTrafficPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    const auto result = BassShowTrafficPrepareDoImpl(ctx, nlg, request, ctx.RequestMeta);

    struct {
        TScenarioHandleContext& Ctx;
        // Common case: request BASS.
        void operator()(const THttpProxyRequest& bassRequest) {
            AddBassRequestItems(Ctx, bassRequest);
        }
        // Irrelevant, Details or Feedback case: exit.
        void operator()(const NScenarios::TScenarioRunResponse& response) {
            Ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        }
    } visitor{ctx};
    std::visit(visitor, result);
}

} // namespace NAlice::NHollywood
