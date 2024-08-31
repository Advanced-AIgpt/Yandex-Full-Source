#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/synchronize_state/service.h>

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/config/config.h>

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>


using namespace NAlice::NCuttlefish;


constexpr TStringBuf EMPTY_CONTEXT = "[]";

namespace {

TLogContext CreateFakeLogContext() {
    return TLogContext(new TSelfFlushLogFrame(nullptr), nullptr);
}

} // namespace


Y_UNIT_TEST_SUITE(Basic) {

    Y_UNIT_TEST(Preprocess) {
        NAppHost::NService::TTestContext serviceCtx(EMPTY_CONTEXT);

        AddProtobufInputItem<NAliceProtocol::TSessionContext>(serviceCtx, "INIT", ITEM_TYPE_SESSION_CONTEXT, R"__(
            ConnectionInfo {
                IpAddress: "10.20.30.40"
            }
        )__");
        AddProtobufInputItem<NAliceProtocol::TEvent>(serviceCtx, "INIT", ITEM_TYPE_SYNCRHONIZE_STATE_EVENT, R"__(
            Header {
                Namespace: SYSTEM
                Name: SYNCHRONIZE_STATE
                MessageId: "00000000-0000-0000-0000-000000000000"
            }
            SyncState {
                AppToken: "this-token-is-not-in-whitelist"
                UserInfo {
                    Uuid: "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
                }
            }
        )__");

        NAppHostServices::SynchronizeStatePreprocess(GetDefaultCuttlefishConfig(), serviceCtx, CreateFakeLogContext());
        const auto items = serviceCtx.GetProtobufItemRefs(NAppHost::EContextItemSelection::Output);

        UNIT_ASSERT(Contains<NAliceProtocol::TSessionContext>(items, ITEM_TYPE_SESSION_CONTEXT, R"__(
            AppToken: "this-token-is-not-in-whitelist"
            InitialMessageId: "00000000-0000-0000-0000-000000000000"
            ConnectionInfo {
                IpAddress: "10.20.30.40"
            }
            UserInfo {
                Uuid: "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
                ICookie: "5906247140874454993"
            }
        )__"));
        UNIT_ASSERT(Contains<NAppHostHttp::THttpRequest>(items, ITEM_TYPE_APIKEYS_HTTP_REQUEST, R"__(
            Scheme: Http
            Path: "/check_key?service_token=speechkitmobile_cad5058d5cf684e2ab5442a77d2a52871aa825ef&key=this-token-is-not-in-whitelist&user_ip=10.20.30.40&ip_v=4"
        )__"));
        UNIT_ASSERT(Contains<NAppHostHttp::THttpRequest>(items, ITEM_TYPE_HTTP_REQUEST_DRAFT, R"__(
            Scheme: Https
            Path: "datasync_http_request@/v1/personality/profile/alisa/settings"
        )__"));
    }

}
