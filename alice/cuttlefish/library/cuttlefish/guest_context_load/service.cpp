#include "service.h"

#include <alice/cuttlefish/library/cuttlefish/common/datasync.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/cuttlefish/guest_context_load/blackbox.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>
#include <apphost/lib/proto_answers/tvm_user_ticket.pb.h>
#include <apphost/lib/proto_answers/http.pb.h>

using namespace NAliceProtocol;

namespace {

    using namespace NAlice::NCuttlefish;
    using namespace NAlice::NCuttlefish::NAppHostServices;

    void TryPutTvmUserTicketToAppHostContext(
        NAppHost::IServiceContext &ctx,
        const TBlackboxClient::TOAuthResponse& blackboxResp,
        TLogContext logContext
    ) {
        if (!blackboxResp.UserTicket) {
            logContext.LogEventInfoCombo<NEvClass::NoTvmUserTicketInBlackboxResponse>();
        } else {
            NAppHostTvmUserTicket::TTvmUserTicket tvmUserTicket;
            tvmUserTicket.SetUserTicket(blackboxResp.UserTicket);

            ctx.AddProtobufItem(tvmUserTicket, ITEM_TYPE_TVM_USER_TICKET);
            logContext.LogEventInfoCombo<NEvClass::SendToAppHostTvmUserTicket>();
        }
    }

    void TryPutBlackboxUidToAppHostContext(
        NAppHost::IServiceContext &ctx,
        const TBlackboxClient::TOAuthResponse& blackboxResp,
        TLogContext logContext
    ) {
        if (!blackboxResp.Uid) {
            logContext.LogEventInfoCombo<NEvClass::NoUidInBlackboxResponse>();
            return;
        }

        NAliceProtocol::TContextLoadBlackboxUid blackboxUid;
        blackboxUid.SetUid(blackboxResp.Uid);

        ctx.AddProtobufItem(blackboxUid, ITEM_TYPE_BLACKBOX_UID);
        logContext.LogEventInfoCombo<NEvClass::SendToAppHostBlackboxUid>();
    }

}

namespace NAlice::NCuttlefish::NAppHostServices {

    void GuestContextLoadBlackboxSetdown(NAppHost::IServiceContext &ctx, TLogContext logContext) {
        TSourceMetrics metrics(ctx, "guest_context_load_blackbox_setdown");

        auto maybeBlackbox = ParseBlackboxResponse(ctx, metrics, logContext);
        if (maybeBlackbox) {

            TryPutTvmUserTicketToAppHostContext(ctx, *maybeBlackbox, logContext);
            TryPutBlackboxUidToAppHostContext(ctx, *maybeBlackbox, logContext);

            // construct request to Datasync
            {
                NAppHostHttp::THttpRequest req = TDatasyncClient::LoadVinsContextsRequest();
                logContext.LogEventInfoCombo<NEvClass::SendToAppHostDatasyncHttpRequest>(req.ShortUtf8DebugString());
                TDatasyncClient::AddUserTicketHeader(req, maybeBlackbox->UserTicket);
                TDatasyncClient::AddUidHeader(req, maybeBlackbox->Uid);

                ctx.AddProtobufItem(std::move(req), ITEM_TYPE_GUEST_DATASYNC_HTTP_REQUEST);

                metrics.PushRate("response", "ok", "blackbox");
                metrics.PushRate("request", "ok", "datasync");
            }

        } else {
            metrics.PushRate("response", "error", "blackbox");
        }
    }

}  // namespace NAlice::NCuttlefish::NAppHostServices
