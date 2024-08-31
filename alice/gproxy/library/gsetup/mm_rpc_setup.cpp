#include "mm_rpc_setup.h"

#include <alice/gproxy/library/gsetup_conv/mm_rpc_request_builder.h>
#include <alice/gproxy/library/gsetup_conv/request_builder.h>

#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/library/client/protos/client_info.pb.h>

#include <contrib/libs/protobuf/src/google/protobuf/wrappers.pb.h>

namespace NGProxy {

bool TGSetupMMRpc::ProcessInputs(NAppHost::IServiceContext& ctx, NGProxy::TMetadata& meta,
                                 NGProxy::GSetupRequestInfo& info, google::protobuf::Any& req,
                                 TLogContext& logContext) {
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

    req = ctx.GetOnlyProtobufItem<google::protobuf::Any>("request");

    return true;
}

void TGSetupMMRpc::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    NGProxy::TMetadata meta;
    NGProxy::GSetupRequestInfo info;
    google::protobuf::Any req;

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

    if (meta.HasApplication()) {
        google::protobuf::StringValue appInfo;
        appInfo.set_value(meta.GetApplication());
        ctx.AddProtobufItem(appInfo, "rpc_datasource_app_info");
    }

    auto requestBuilder =
        TMMRpcRequestBuilder(info.GetMethod())
            .SetRequestId(meta.GetRequestId())
            .SetRandomSeed(meta.GetRandomSeed())
            .SetServerTimeMs(meta.GetServerTimeMs())
            .SetLaasRegion(contextLoadResponse.GetLaasResponse())
            .SetTestIds(contextLoadResponse.GetFlagsInfo())
            .SetExperiments(contextLoadResponse.GetFlagsInfo().GetVoiceFlags())
            .SetSupportedFeatures(meta.GetSupportedFeatures())
            .SetUnsupportedFeatures(meta.GetUnsupportedFeatures())
            .ConstructApplication(session)
            .SetRequest(req);

    ctx.AddProtobufItem(std::move(requestBuilder).Build(), "mm_rpc_request");

    ctx.Flush();
}

}   // namespace NGProxy
