#include "request_meta_setup.h"

#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <alice/gproxy/library/gsetup_conv/request_builder.h>

#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <apphost/lib/proto_answers/tvm_user_ticket.pb.h>

namespace NGProxy {

    void TGSetupRequestMeta::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
        if (!ctx.HasProtobufItem("request_info")) {
            AddError(ctx, "unknown", "GPROXY_REQUEST_META_SETUP", "no request_info item in apphost context", logContext);
            return;
        }

        NGProxy::GSetupRequestInfo info = ctx.GetOnlyProtobufItem<NGProxy::GSetupRequestInfo>("request_info");

        if (!ctx.HasProtobufItem("metadata")) {
            AddError(ctx, info.GetMethod(), "GPROXY_REQUEST_META_SETUP", "no metadata item in apphost context", logContext);
            return;
        }

        NGProxy::TMetadata meta = ctx.GetOnlyProtobufItem<NGProxy::TMetadata>("metadata");

        NAlice::NScenarios::TRequestMeta scenarioMeta;

        scenarioMeta.SetLang(meta.GetLanguage());
        scenarioMeta.SetRequestId(meta.GetRequestId());
        scenarioMeta.SetUserLang(meta.GetUserLang());
        scenarioMeta.SetClientIP(meta.GetIpAddr());

        if (meta.GetInternalRequest()) {
            scenarioMeta.SetRandomSeed(meta.GetRandomSeed());
            scenarioMeta.SetUserTicket(meta.GetUserTicket());
        }

        if (meta.HasOAuthToken()) {
            scenarioMeta.SetOAuthToken(meta.GetOAuthToken());
        }

        if (ctx.HasProtobufItem("tvm_user_ticket")) {
            NAppHostTvmUserTicket::TTvmUserTicket ticket = ctx.GetOnlyProtobufItem<NAppHostTvmUserTicket::TTvmUserTicket>("tvm_user_ticket");
            scenarioMeta.SetUserTicket(ticket.GetUserTicket());
        }

        ctx.AddProtobufItem(scenarioMeta, "scenario_request_meta");
        ctx.Flush();
    }

} // namespace NGProxy
