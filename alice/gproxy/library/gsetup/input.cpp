#include "input.h"

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/gproxy/library/protos/metadata.pb.h>
#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>


namespace NGProxy {

bool TGSetupInput::ProcessInputs(NAppHost::IServiceContext& ctx, NGProxy::TMetadata& metadata, NGProxy::GSetupRequestInfo& request, TLogContext& logContext) {
    if (!ctx.HasProtobufItem("request_info")) {
        AddError(ctx, "unknown", "GSETUP_INPUT", "no request item in apphost context", logContext);
        return false;
    }

    request = ctx.GetOnlyProtobufItem<NGProxy::GSetupRequestInfo>("request_info");

    if (!ctx.HasProtobufItem("metadata")) {
        AddError(ctx, request.GetMethod(), "GSETUP_INPUT", "no metadata item in apphost context", logContext);
        return false;
    }

    metadata = ctx.GetOnlyProtobufItem<NGProxy::TMetadata>("metadata");

    return true;
}


void TGSetupInput::Handle(NAppHost::IServiceContext& ctx, TLogContext& logContext) {
    NGProxy::TMetadata meta;
    NGProxy::GSetupRequestInfo req;

    if (!ProcessInputs(ctx, meta, req, logContext)) {
        return;
    }

    LogRequestInfo(req, meta, "GSETUP_INPUT", logContext);

    //
    //  Validate metadata fields
    //
    if (!meta.HasSessionId()) {
        AddError(ctx, req.GetMethod(), "GPROXY_INPUT", "no 'x-ya-session-id' in client metadata", logContext);
        return;
    }

    if (!meta.HasRequestId()) {
        AddError(ctx, req.GetMethod(), "GPROXY_INPUT", "no 'x-ya-request-id' in client metadata", logContext);
        return;
    }

    if (!meta.HasAppId()) {
        AddError(ctx, req.GetMethod(), "GPROXY_INPUT", "no 'x-ya-app-id' in client metadata", logContext);
        return;
    }

    if (!meta.HasAppType()) {
        AddError(ctx, req.GetMethod(), "GPROXY_INPUT", "no 'x-ya-app-type' in client metadata", logContext);
        return;
    }

    if (!meta.HasDeviceId()) {
        AddError(ctx, req.GetMethod(), "GPROXY_INPUT", "no 'x-ya-device-id' in client metadata", logContext);
        return;
    }

    if (!meta.HasUuid()) {
        AddError(ctx, req.GetMethod(), "GPROXY_INPUT", "no 'x-ya-uuid' in client metadata", logContext);
        return;
    }

    if (!meta.HasLanguage()) {
        AddError(ctx, req.GetMethod(), "GPROXY_INPUT", "no 'x-ya-language' in client metadata", logContext);
        return;
    }


    //
    //  Fill TSessionContext for requesting megamind context
    //
    NAliceProtocol::TSessionContext SessionContext;
    SessionContext.SetSessionId(meta.GetSessionId());
    SessionContext.SetAppId(meta.GetAppId());
    SessionContext.SetAppType(meta.GetAppType());
    SessionContext.SetAppLang(meta.GetLanguage());
    SessionContext.SetLanguage(meta.GetLanguage());

    //
    //  Fill connection info
    //
    if (meta.HasIpAddr()) {
        NAliceProtocol::TConnectionInfo *conn = SessionContext.MutableConnectionInfo();
        conn->SetIpAddress(meta.GetIpAddr());
    } else {
        logContext.LogEventInfoCombo<NEvClass::GSetupWarning>("no client IP was provided");
    }

    //
    //  Fill user info
    //
    {
        NAliceProtocol::TUserInfo *user = SessionContext.MutableUserInfo();
        if (meta.HasOAuthToken()) {
            user->SetAuthToken(meta.GetOAuthToken());
            user->SetAuthTokenType(NAliceProtocol::TUserInfo_ETokenType_OAUTH);
        } else {
            logContext.LogEventInfoCombo<NEvClass::GSetupWarning>("no oauth token was provided");
        }
        user->SetUuid(meta.GetUuid());
        user->SetVinsApplicationUuid(meta.GetUuid());
    }

    //
    //  Fill device info
    //
    {
        NAliceProtocol::TDeviceInfo *device = SessionContext.MutableDeviceInfo();
        device->SetDeviceId(meta.GetDeviceId());
    }

    ctx.AddProtobufItem(SessionContext, "session_context");

    ctx.AddFlag(NAlice::NCuttlefish::EDGE_FLAG_LOAD_CONTEXT_SOURCE_DATASYNC);
    ctx.AddFlag(NAlice::NCuttlefish::EDGE_FLAG_LOAD_CONTEXT_SOURCE_FLAGS_JSON);
    ctx.AddFlag(NAlice::NCuttlefish::EDGE_FLAG_LOAD_CONTEXT_SOURCE_IOT_USER_INFO);
    ctx.AddFlag(NAlice::NCuttlefish::EDGE_FLAG_LOAD_CONTEXT_SOURCE_MEMENTO);

    ctx.Flush();
}

}   // namespace NGProxy
