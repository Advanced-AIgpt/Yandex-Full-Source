#include "handler.h"

#include <alice/megamind/library/apphost_request/response.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/registry/registry.h>

#include <alice/library/network/common.h>
#include <alice/library/version/version.h>

#include <apphost/lib/common/constants.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NMegamind {
namespace {

const TString PATH_PING = "/ping";
const TString PATH_VERSION = "/version";
const TString PATH_VERSION_JSON = "/version_json";

const TString PATH_RELOAD_LOGS = "/reload_logs";

class TLogManager {
public:
    TLogManager(IGlobalCtx& globalCtx, TMaybe<NInfra::TLogger>& udpLogger, TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient)
        : RTLogClient_{globalCtx.RTLogClient()}
        , MegamindAnalyticsLog_{globalCtx.MegamindAnalyticsLog()}
        , MegamindProactivityLog_{globalCtx.MegamindProactivityLog()}
        , Logger_{globalCtx.BaseLogger()}
        , UdpLogger_(udpLogger)
        , UdpClient_(udpClient)
    {
    }

    TString Rotate() {
        TStringBuilder error{};

        try {
            RTLogClient_.Rotate();
        } catch (...) {
            error << "RTLog exception: " << TBackTrace::FromCurrentException().PrintToString() << '\n';
        }
        try {
            MegamindAnalyticsLog_.ReopenLog();
        } catch (...) {
            error << "MegamindAnalyticsLog exception: " << TBackTrace::FromCurrentException().PrintToString() << '\n';
        }
        try {
            MegamindProactivityLog_.ReopenLog();
        } catch (...) {
            error << "MegamindProactivityLog exception: " << TBackTrace::FromCurrentException().PrintToString() << '\n';
        }
        try {
            if (UdpLogger_.Defined()) {
                UdpLogger_->ReopenLog();
            }
        } catch (...) {
            error << "UdpLogger exception: " << TBackTrace::FromCurrentException().PrintToString() << '\n';
        }
        try {
            if (UdpClient_.Defined()) {
                UdpClient_->ReopenLogs();
            }
        } catch (...) {
            error << "UdpClient exception: " << TBackTrace::FromCurrentException().PrintToString() << '\n';
        }

        if (error.empty()) {
            return "OK";
        }
        LOG_ERROR(Logger_) << "Exception during rotation: " << error;
        return error;
    }

private:
    NAlice::TRTLogClient& RTLogClient_;
    TLog& MegamindAnalyticsLog_;
    TLog& MegamindProactivityLog_;
    NAlice::TRTLogger& Logger_; // Only for logging error, not for rotating.
    TMaybe<NInfra::TLogger>& UdpLogger_;
    TMaybe<NUdpClickMetrics::TSelfBalancingClient>& UdpClient_;
};

TString MakeNotImplementedAnswer(const TString& path) {
    return TStringBuilder{} << "Handler " << path << " is not implemented.";
}

void UtilityHandler(IGlobalCtx& globalCtx, NAppHost::IServiceContext& ctx) {
    auto logger = globalCtx.RTLogger(/* token= */ "", /* session= */ false);
    TItemProxyAdapter itemProxyAdapter{ctx, logger, globalCtx, /* useStreaming= */ false};
    const auto& httpReq = itemProxyAdapter.GetFromContext<NAppHostHttp::THttpRequest>(NAppHost::PROTO_HTTP_REQUEST);

    TAppHostHttpResponse response{itemProxyAdapter};
    if (!httpReq.IsSuccess()) {
        response.SetHttpCode(HttpCodes::HTTP_NOT_IMPLEMENTED)
                .SetContentType(NContentTypes::TEXT_PLAIN)
                .SetContent("Failed to parse incomming http request");
    }

    if (httpReq.Value().GetPath().StartsWith(PATH_VERSION_JSON)) {
        FillJsonVersionData<IHttpResponse>(response);
    } else if (httpReq.Value().GetPath().StartsWith(PATH_VERSION)) {
        FillVersionData<IHttpResponse>(response);
    } else {
        response.SetHttpCode(HttpCodes::HTTP_INTERNAL_SERVER_ERROR)
                .SetContentType(NContentTypes::TEXT_PLAIN)
                .SetContent(MakeNotImplementedAnswer(httpReq.Value().GetPath()));
    }
    response.Flush();
}

} // namespace

namespace NImpl {

void UtilityHandlerHttp(const TString& path,
                        IGlobalCtx& globalCtx,
                        TMaybe<NInfra::TLogger>& udpLogger,
                        TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient,
                        const NNeh::IRequestRef& req)
{
    NNeh::TDataSaver ds;
    if (path == PATH_PING) {
        ds << "pong";
    } else if (path == PATH_RELOAD_LOGS) {
        TLogManager logManager{globalCtx, udpLogger, udpClient};
        ds << logManager.Rotate();
    } else if (path == PATH_VERSION) {
        ds << GetProgramSvnVersion();
    } else if (path == PATH_VERSION_JSON) {
        ds << JsonToString(CreateVersionData());
    } else {
        req->SendError(NNeh::IRequest::TResponseError::NotImplemented, MakeNotImplementedAnswer(path));
        return;
    }

    req->SendReply(ds);
}

} // namespace NImpl

void RegisterAppHostUtilityHandlers(IGlobalCtx& globalCtx, TRegistry& registry) {
    registry.Add("/utility", [&globalCtx](NAppHost::IServiceContext& ctx) {
        UtilityHandler(globalCtx, ctx);
    });
}

void RegisterAppHostHttpUtilityHandlers(IGlobalCtx& globalCtx,
                                        TMaybe<NInfra::TLogger>& udpLogger,
                                        TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient,
                                        TRegistry& registry)
{
    const TVector<TString> httpPaths {
        PATH_PING,
        PATH_VERSION,
        PATH_VERSION_JSON,
        PATH_RELOAD_LOGS,
    };

    for (const auto& path : httpPaths) {
        registry.Add(path, [path, &globalCtx, &udpLogger, &udpClient](const NNeh::IRequestRef& req) {
            NImpl::UtilityHandlerHttp(path, globalCtx, udpLogger, udpClient, req);
        });
    }
}

} // namespace NAlice::NMegamind
