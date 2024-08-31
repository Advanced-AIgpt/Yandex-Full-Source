#include "mm_rpc_output.h"

#include <alice/gproxy/library/gsetup_conv/response_builder.h>

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/context_save/client/starter.h>
#include <alice/library/json/json.h>
#include <alice/megamind/protos/grpc_request/response.pb.h>
#include <alice/protos/api/rpc/status.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/json/json_reader.h>


namespace NGProxy {

namespace {

void SetupContextSaveRequest(const NAlice::NRpc::TRpcResponseProto& /*response*/, const NGProxy::TMetadata& meta,
                             NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    NAlice::NCuttlefish::NContextSaveClient::TContextSaveStarter starter;
    starter.SetRequestId(meta.GetRequestId());
    starter.AddClientExperiment("use_memento", "1");
    starter.SetAppId(meta.GetAppId());
    // starter.SetPuid(???);  // notificator has no use here for now.

    // TODO https://st.yandex-team.ru/MEGAMIND-3887
    const uint32_t requestSize = 0; //starter.AddDirectives(response.GetServerDirectives());

    if (requestSize == 0) {
        logContext.LogEventInfoCombo<NEvClass::GProxySourceRequest>(
            "CONTEXT_SAVE",
            "No request (no directives found in mm response)"
        );
    } else {
        logContext.LogEventInfoCombo<NEvClass::GProxySourceRequest>(
            "CONTEXT_SAVE",
            starter.GetRequestRef().ShortUtf8DebugString());
        std::move(starter).Finalize(ctx, "context_save_request", "");
        ctx.AddFlag(NAlice::NCuttlefish::EDGE_FLAG_SAVE_CONTEXT_SOURCE_MEMENTO);
    }
}

} // namespace

void TGSetupMMRpcOutput::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    if (!ctx.HasProtobufItem("request_info")) {
        AddError(ctx, "unknown", "GPROXY_OUTPUT", "no 'request_info' item", logContext);
        return;
    }
    NGProxy::GSetupRequestInfo info = ctx.GetOnlyProtobufItem<NGProxy::GSetupRequestInfo>("request_info");

    if (!ctx.HasProtobufItem("metadata")) {
        AddError(ctx, info.GetMethod(), "GPROXY_OUTPUT", "no 'metadata' item", logContext);
    }

    NGProxy::TMetadata meta = ctx.GetOnlyProtobufItem<NGProxy::TMetadata>("metadata");

    if (!ctx.HasProtobufItem("mm_rpc_response")) {
        AddError(ctx, info.GetMethod(), "GPROXY_OUTPUT", "no 'mm_rpc_response' item", logContext);
        return;
    }

    const auto response = ctx.GetOnlyProtobufItem<NAlice::NRpc::TRpcResponseProto>("mm_rpc_response");

    // TODO(nkodosov) another response fields?
    if (response.HasError()) {
        AddError(ctx, info.GetMethod(), "GPROXY_OUTPUT", response.GetError().GetMessage(), logContext);
    } else {
        SetupContextSaveRequest(response, meta, ctx, logContext);
        ctx.AddProtobufItem(response.GetResponseBody(), "response");
    }

    ctx.Flush();
}

}   // namespace NGProxy
