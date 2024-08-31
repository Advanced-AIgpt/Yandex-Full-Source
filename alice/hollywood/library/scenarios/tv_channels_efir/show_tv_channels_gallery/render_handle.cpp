#include "render_handle.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/library/channel_item.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/library/util.h>
#include <alice/library/video_common/hollywood_helpers/proxy_request_builder.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

#include <alice/library/video_common/defs.h>
#include <alice/protos/data/video/video.pb.h>

#include <util/generic/vector.h>


using namespace NAlice::NScenarios;

namespace {

TVector<NAlice::TVideoItem> ParseQuasarReponse(const NJson::TJsonValue& response) {
    TVector<NAlice::TVideoItem> videoItems;
    if (const auto* set = response.GetValueByPath("set"); set && set->IsArray()) {
        for (const auto& item : set->GetArray()) {
            auto videoItem = NAlice::NHollywood::NTvChannelsEfir::ParseChannelJson(item);
            if (videoItem.Defined()) {
                videoItems.emplace_back(std::move(*videoItem));
            }
        }
    }
    return videoItems;
}

} // namespace

namespace NAlice::NHollywood::NTvChannelsEfir {

void ShowTvChannelsGalleryRenderHandle(TScenarioHandleContext& ctx) {

    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    bool isTvPlugged = request.ClientInfo().IsSmartSpeaker() && request.BaseRequestProto().GetInterfaces().GetIsTvPlugged();
    if (!isTvPlugged) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    if (const auto expFlag = request.ExpFlag(NVideoCommon::FLAG_TV_CHANNELS_WEBVIEW); expFlag.Defined()) {
        return;
    }

    const auto quasarResponse =
        RetireHttpResponseJsonMaybe(ctx, VIDEO_QUASAR_RESPONSE_ITEM, VIDEO_QUASAR_REQUEST_RTLOG_TOKEN_ITEM);

    if (!quasarResponse.Defined()) {
        NVideoCommon::AddIrrelevantResponse(ctx);
        return;
    }

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);

    auto videoItems = ParseQuasarReponse(*quasarResponse);

    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    if (videoItems.empty()) {
        TNlgData nlgData(ctx.Ctx.Logger(), request);
        nlgData.AddAttention(NVideoCommon::ATTENTION_EMPTY_SEARCH_GALLERY);
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_NAME, "render_result", /* buttons = */ {}, nlgData);
        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
        NVideoCommon::FillAnalyticsInfo(bodyBuilder, ANALYTICS_INTENT_NAME, ANALYTICS_PRODUCT_SCENARIO_NAME);
        return;
    }

    TDirective oneOfDirective;
    auto& directive = *oneOfDirective.MutableShowTvGalleryDirective();
    for (auto& videoItem : videoItems) {
        *directive.AddItems() = std::move(videoItem);
    }
    directive.SetName("show_tv_gallery");
    bodyBuilder.AddDirective(std::move(oneOfDirective));
    TNlgData nlgData(ctx.Ctx.Logger(), request);
    nlgData.AddAttention(NVideoCommon::ATTENTION_SHOW_TV_GALLERY);
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_NAME, "render_result", /* buttons = */ {}, nlgData);
    NVideoCommon::FillAnalyticsInfo(bodyBuilder, ANALYTICS_INTENT_NAME, ANALYTICS_PRODUCT_SCENARIO_NAME);
    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
