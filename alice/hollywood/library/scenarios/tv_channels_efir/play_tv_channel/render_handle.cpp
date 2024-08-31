#include "render_handle.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/library/util.h>

#include <alice/library/video_common/frontend_vh_helpers/video_item_helper.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannelsEfir {

void PlayTvChannelRenderHandle(TScenarioHandleContext& ctx) {
    const auto vhResponse =
        RetireHttpResponseJsonMaybe(ctx, VH_PLAYER_RESPONSE_ITEM, VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM);

    if (!vhResponse) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    const auto videoItemHelper = NVideoCommon::TVideoItemHelper::TryMakeFromVhPlayerResponse(*vhResponse);
    if (!videoItemHelper.Defined()) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    TDirective oneOfDirective;
    *oneOfDirective.MutableVideoPlayDirective() = videoItemHelper->MakeVideoPlayDirective(request);
    bodyBuilder.AddDirective(std::move(oneOfDirective));
    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.AddAttention(NAlice::NVideoCommon::ATTENTION_AUTOPLAY);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_NAME, "play_tv_channel", /* buttons = */ {}, nlgData);
    NVideoCommon::FillAnalyticsInfo(bodyBuilder, ANALYTICS_INTENT_NAME, ANALYTICS_PRODUCT_SCENARIO_NAME);

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
