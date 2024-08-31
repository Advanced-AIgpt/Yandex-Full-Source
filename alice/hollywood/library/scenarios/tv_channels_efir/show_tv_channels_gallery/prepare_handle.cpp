#include "prepare_handle.h"


#include <alice/hollywood/library/scenarios/mordovia_video_selection/util.h>
#include <alice/hollywood/library/scenarios/tv_channels_efir/library/util.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/bass/libs/client/debug_flags.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/network/headers.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/mordovia_webview_defs.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/library/video_common/video_helper.h>
#include <alice/protos/data/video/video.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannelsEfir {

namespace {

THttpProxyRequest PrepareVideoQuasarRequest(const TScenarioRunRequestWrapper& request,
                                                     const TScenarioHandleContext& ctx) {

    NVideoCommon::TProxyRequestBuilder requestBuilder(ctx, request);

    NVideoCommon::AddIpregParam(requestBuilder, request);

    const auto serverTimeMs = TInstant::MilliSeconds(request.ServerTimeMs());
    const auto currentTime = ToString(serverTimeMs.Seconds());
    requestBuilder.AddCgiParam("end_date__from", currentTime);
    requestBuilder.AddCgiParam("start_time__to", currentTime);
    requestBuilder.AddCgiParam("now", currentTime);
    requestBuilder.AddCgiParam("from", NAlice::NVideoCommon::QUASAR_FROM_ID);
    requestBuilder.AddCgiParam("service", "ya-station");
    requestBuilder.AddCgiParam("disable_renderer", "");

    if (const auto expFlag = request.ExpFlag(NAlice::NVideoCommon::DISABLE_PERSONAL_TV_CHANNEL); !expFlag.Defined()) {
        requestBuilder.AddCgiParam("add_personal_channel", "true");
    }

    if (const auto expFlag = request.ExpFlag(NAlice::NExperiments::EXP_TV_SHOW_RESTREAMED_CHANNELS_IN_GALLERY); expFlag.Defined()) {
        if (const auto flagValue = request.ExpFlag(NAlice::NExperiments::EXP_TV_RESTREAMED_CHANNELS_LIST); flagValue.Defined()) {
            requestBuilder.AddCgiParam("restreamed_channels", *flagValue);
        }
    }

    requestBuilder.SetScheme(NAppHostHttp::THttpRequest::None);

    const auto& requestOptions = request.BaseRequestProto().GetOptions();
    requestBuilder.AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestOptions.GetUserAgent());

    return requestBuilder.Build();
}

} // namespace

void ShowTvChannelsGalleryPrepareHandle(TScenarioHandleContext& ctx) {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    bool isTvPlugged = request.ClientInfo().IsSmartSpeaker() && request.BaseRequestProto().GetInterfaces().GetIsTvPlugged();
    if (!isTvPlugged) {
        return;
    }
    if (const auto expFlag = request.ExpFlag(NVideoCommon::FLAG_TV_CHANNELS_WEBVIEW); !expFlag.Defined()) {
        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TRunResponseBuilder builder(&nlgWrapper);

        const THttpProxyRequest quasarRequest = PrepareVideoQuasarRequest(request, ctx);
        AddHttpRequestItems(ctx, quasarRequest, VIDEO_QUASAR_REQUEST_ITEM, VIDEO_QUASAR_REQUEST_RTLOG_TOKEN_ITEM);
    } else {
        TDirective oneOfDirective;
        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        TRunResponseBuilder builder(&nlgWrapper);
        auto &bodyBuilder = builder.CreateResponseBodyBuilder();
        const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

        TCgiParameters cgi = NVideoCommon::GetDefaultWebviewCgiParams(request);

        NVideoCommon::AddIpregParam(cgi, request);

        if (const auto expFlag = request.ExpFlag(NBASS::DEBUG_FLAG_TV_PRESTABLE_CHANNELS); expFlag.Defined()) {
            cgi.InsertUnescaped("service", "ya-station-prestable");
        } else {
            cgi.InsertUnescaped("service", "ya-station");
        }
        cgi.InsertUnescaped("from", NAlice::NVideoCommon::QUASAR_FROM_ID);

        if (const auto expFlag = request.ExpFlag(NAlice::NVideoCommon::DISABLE_PERSONAL_TV_CHANNEL); !expFlag.Defined()) {
            cgi.InsertUnescaped("add_personal_channel", "true");
        }

        if (const auto expFlag = request.ExpFlag(NAlice::NExperiments::EXP_TV_SHOW_RESTREAMED_CHANNELS_IN_GALLERY); expFlag.Defined()) {
            if (const auto flagValue = request.ExpFlag(NAlice::NExperiments::EXP_TV_RESTREAMED_CHANNELS_LIST); flagValue.Defined()) {
                cgi.InsertUnescaped("restreamed_channels", *flagValue);
            }
        }
        TString host = ToString(NAlice::NVideoCommon::DEFAULT_WEBVIEW_VIDEO_HOST);
        if (const auto expHost = request.ExpFlag(NAlice::NVideoCommon::FLAG_WEBVIEW_VIDEO_HOST); expHost.Defined()) {
            host = *expHost;
        }
        TString path = ToString(NAlice::NVideoCommon::DEFAULT_CHANNELS_PATH);
        if (const auto expPath = request.ExpFlag(NAlice::NVideoCommon::FLAG_WEBVIEW_CHANNELS_PATH); expPath.Defined()) {
            path = *expPath;
        }
        TString splashDiv = ToString(NAlice::NVideoCommon::DEFAULT_CHANNELS_SPLASH);
        if (const auto expSplash = request.ExpFlag(NAlice::NVideoCommon::FLAG_WEBVIEW_CHANNELS_SPLASH); expSplash.Defined()) {
            splashDiv = *expSplash;
        }

        const auto& delimiter = path.Contains("?") ? "&" : "?";
        TString pathWithParams = TStringBuilder() << path << delimiter << cgi.Print();

        NMordovia::AddWebViewCommand(request, bodyBuilder, host, pathWithParams, splashDiv, /* oldViewKey */ TStringBuf{"video"});
        TNlgData nlgData(ctx.Ctx.Logger(), request);
        nlgData.AddAttention(NVideoCommon::ATTENTION_SHOW_TV_GALLERY);
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_NAME, "render_result", /* buttons = */ {}, nlgData);
        NVideoCommon::FillAnalyticsInfo(bodyBuilder, ANALYTICS_INTENT_NAME, ANALYTICS_PRODUCT_SCENARIO_NAME);

        auto response = std::move(builder).BuildResponse();
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
