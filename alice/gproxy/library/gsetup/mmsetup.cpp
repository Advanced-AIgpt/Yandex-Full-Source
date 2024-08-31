#include "mmsetup.h"

#include <alice/cuttlefish/library/protos/context_load.pb.h>

#include <alice/gproxy/library/gsetup_conv/request_builder.h>


namespace NGProxy {

bool TGSetupMM::ProcessInputs(NAppHost::IServiceContext& ctx, NGProxy::TMetadata& meta, NGProxy::GSetupRequestInfo& info, NGProxy::GSetupRequest& req, TLogContext& logContext) {
    if (!ctx.HasProtobufItem("request_info")) {
        AddError(ctx, "unknown", "GPROXY_MM_SETUP", "no request_info item in apphost context", logContext);
        return false;
    }

    info = ctx.GetOnlyProtobufItem<NGProxy::GSetupRequestInfo>("request_info");

    if (!ctx.HasProtobufItem("metadata")) {
        AddError(ctx, info.GetMethod(), "GPROXY_MM_SETUP", "no metadata item in apphost context", logContext);
        return false;
    }

    meta = ctx.GetOnlyProtobufItem<NGProxy::TMetadata>("metadata");

    if (!ctx.HasProtobufItem("request")) {
        AddError(ctx, info.GetMethod(), "GPROXY_MM_SETUP", "no request item in apphost context", logContext);
        return false;
    }

    req = ctx.GetOnlyProtobufItem<NGProxy::GSetupRequest>("request");

    return true;
}

void TGSetupMM::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    NGProxy::TMetadata meta;
    NGProxy::GSetupRequestInfo info;
    NGProxy::GSetupRequest req;

    if (!ProcessInputs(ctx, meta, info, req, logContext)) {
        return;
    }

    LogRequestInfo(info, meta, "GPROXY_MM_SETUP", logContext);

    if (!ctx.HasProtobufItem("session_context")) {
        AddError(ctx, info.GetMethod(), "GPROXY_MM_SETUP", "no 'session_context' item found", logContext);
        return;
    }

    if (!ctx.HasProtobufItem("context_load_response")) {
        AddError(ctx, info.GetMethod(), "GPROXY_MM_SETUP", "no 'context_load_response' item found", logContext);
        return;
    }

    const NAliceProtocol::TSessionContext session =
        ctx.GetOnlyProtobufItem<NAliceProtocol::TSessionContext>("session_context");
    const NAliceProtocol::TContextLoadResponse contextLoadResponse =
        ctx.GetOnlyProtobufItem<NAliceProtocol::TContextLoadResponse>("context_load_response");

    if (session.GetUserInfo().HasAuthToken() && !contextLoadResponse.HasUserTicket()) {
        AddError(ctx, info.GetMethod(), "GPROXY_MM_SETUP", "seems like oauth token is invalid or BB request failed", logContext);
        return;
    }


    try {
        TRequestBuilder builder;
        builder.SetSession(session);
        builder.SetContext(contextLoadResponse);
        builder.SetGrpcData(meta, info, req);

        NAppHostHttp::THttpRequest httpReq = builder.Build();

        if (NRTLog::TRequestLoggerPtr rtlog = logContext.RtLogPtr(); rtlog != nullptr) {
            TString token = rtlog->LogChildActivationStarted(false, "mm_request");
            auto* header = httpReq.AddHeaders();
            header->SetName("X-Yandex-Req-Id");
            header->SetValue(std::move(token));
        }

        if (meta.GetExtraLogging()) {
            logContext.LogEventInfoCombo<NEvClass::GProxySourceRequest>("MEGAMIND_HTTP", httpReq.ShortUtf8DebugString());
        }

        ctx.AddProtobufItem(httpReq, "mm_request");
    } catch (const std::exception& ex) {
        logContext.LogEventInfoCombo<NEvClass::GSetupError>(ex.what());
    }

    ctx.Flush();
}

}   // namespace NGProxy
