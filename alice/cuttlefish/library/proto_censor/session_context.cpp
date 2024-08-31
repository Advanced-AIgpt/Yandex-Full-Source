#include "session_context.h"

#include "megamind.h"

void NAlice::NCuttlefish::Censore(NAliceProtocol::TSessionContext& ctx) {
    if (ctx.HasUserInfo()) {
        NAliceProtocol::TUserInfo ui;
        ctx.MutableUserInfo()->Swap(&ui);
        if (ui.HasUuidKind()) {
            ctx.MutableUserInfo()->SetUuidKind(ui.GetUuidKind());
        }
    }
    if (ctx.HasRequestBase()) {
         NAlice::NCuttlefish::Censore(*ctx.MutableRequestBase());
    }
    if (ctx.HasConnectionInfo()) {
        NAliceProtocol::TConnectionInfo ci;
        ctx.MutableConnectionInfo()->Swap(&ci);
        if (ci.HasIpAddress()) {
            ctx.MutableConnectionInfo()->SetIpAddress(ci.GetIpAddress());
        }
        if (ci.HasPredefinedIpAddress()) {
            ctx.MutableConnectionInfo()->SetPredefinedIpAddress(ci.GetPredefinedIpAddress());
        }
        *ctx.MutableConnectionInfo()->MutableTestIds() = ci.GetTestIds();
        if (ci.HasOrigin()) {
            ctx.MutableConnectionInfo()->SetOrigin(ci.GetOrigin());
        }
        if (ci.HasUserAgent()) {
            ctx.MutableConnectionInfo()->SetUserAgent(ci.GetUserAgent());
        }
    }
}
