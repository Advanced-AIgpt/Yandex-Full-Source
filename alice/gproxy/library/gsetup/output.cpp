#include "output.h"

#include <apphost/lib/proto_answers/http.pb.h>

#include <alice/gproxy/library/gsetup_conv/response_builder.h>

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/context_save/client/starter.h>

#include <alice/library/json/json.h>

#include <library/cpp/json/json_reader.h>


namespace NGProxy {


void TGSetupOutput::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    if (!ctx.HasProtobufItem("request_info")) {
        AddError(ctx, "unknown", "GPROXY_OUTPUT", "no 'request_info' item", logContext);
        return;
    }
    NGProxy::GSetupRequestInfo info = ctx.GetOnlyProtobufItem<NGProxy::GSetupRequestInfo>("request_info");

    if (!ctx.HasProtobufItem("metadata")) {
        AddError(ctx, info.GetMethod(), "GPROXY_OUTPUT", "no 'metadata' item", logContext);
    }
    NGProxy::TMetadata meta = ctx.GetOnlyProtobufItem<NGProxy::TMetadata>("metadata");

    LogRequestInfo(info, meta, "GPROXY_OUTPUT", logContext);

    if (!ctx.HasProtobufItem("mm_response")) {
        AddError(ctx, info.GetMethod(), "GPROXY_OUTPUT", "no 'mm_response' item", logContext);
        return;
    }

    NAppHostHttp::THttpResponse response = ctx.GetOnlyProtobufItem<NAppHostHttp::THttpResponse>("mm_response");

    if (meta.GetExtraLogging()) {
        logContext.LogEventInfoCombo<NEvClass::GProxySourceResponse>("MEGAMIND_HTTP", response.DebugString());
    }

    if (NJson::TJsonValue mmResponseJson; NJson::ReadJsonTree(response.GetContent(), &mmResponseJson, false)) {
        SetupContextSaveRequest(ctx, meta, mmResponseJson, logContext);
        BuildGSetupResponse(ctx, meta, info, std::move(mmResponseJson), logContext);
    } else {
        AddError(ctx, info.GetMethod(), "GPROXY_OUTPUT", "failed to parse megamind response to json", logContext);
    }

    ctx.Flush();
}

void TGSetupOutput::SetupContextSaveRequest(
    NAppHost::IServiceContext& ctx,
    const NGProxy::TMetadata& meta,
    const NJson::TJsonValue& mmResponse,
    TLogContext& logContext
) const {
    NAlice::TSpeechKitResponseProto mmResponseProto;
    const google::protobuf::util::Status castStatus = NAlice::JsonToProto(
        mmResponse,
        mmResponseProto,
        /* validateUtf8 = */ false,
        /* ignoreUnknownFields = */ true
    );

    if (!castStatus.ok()) {
        logContext.LogEventInfoCombo<NEvClass::GProxySourceRequest>(
            "CONTEXT_SAVE",
            "No request (unable to parse mm response to protobuf)"
        );
    } else {
        NAlice::NCuttlefish::NContextSaveClient::TContextSaveStarter starter;

        starter.SetRequestId(meta.GetRequestId());
        starter.AddClientExperiment("use_memento", "1");
        starter.SetAppId(meta.GetAppId());
        // starter.SetPuid(???);  // notificator has no use here for now.

        const uint32_t requestSize = starter.AddDirectives(mmResponseProto);

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
}

void TGSetupOutput::BuildGSetupResponse(
    NAppHost::IServiceContext& ctx,
    const NGProxy::TMetadata& meta,
    const NGProxy::GSetupRequestInfo& info,
    NJson::TJsonValue mmResponse,
    TLogContext& logContext
) {
    try {
        TResponseBuilder builder;

        builder.SetResponse(std::move(mmResponse));
        builder.SetRequestInfo(meta, info);

        NGProxy::GSetupResponse out = builder.Build();

        ctx.AddProtobufItem(out, "response");
    } catch (const std::exception& ex) {
        AddError(ctx, info.GetMethod(), "GPROXY_OUTPUT", ex.what(), logContext);
    }
}


}   // namespace NGProxy
