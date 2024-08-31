#include "grpc_finalize_handler.h"

#include "common.h"

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/version/version.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/protos/grpc_request/request.pb.h>
#include <alice/megamind/protos/grpc_request/response.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <contrib/libs/protobuf/src/google/protobuf/wrappers.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NRpc;
using namespace NAlice::NMegamind::NTesting;
using namespace NAlice::NScenarios;

Y_UNIT_TEST_SUITE_F(AppHostMegamindRpcFinalizeHandler, TAppHostFixture) {
    Y_UNIT_TEST(TestRpcHandlerFinalize) {
        auto ahCtx = CreateAppHostContext();

        TRpcFinalizeNodeHandler handler{GlobalCtx, /* useAppHostStreaming= */ false};

        NAlice::NRpc::TRpcRequestProto request;
        request.MutableMeta()->SetRequestId("request_id");
        ahCtx.TestCtx().AddProtobufItem(request, MM_RPC_REQUEST_ITEM_NAME, NAppHost::EContextItemKind::Input);

        NScenarios::TScenarioRpcResponse response;
        google::protobuf::StringValue payload;
        payload.set_value("payload");
        response.MutableResponseBody()->PackFrom(payload);
        response.MutableAnalyticsInfo()->SetProductScenarioName("psn");
        response.SetVersion(NAlice::VERSION_STRING);
        response.AddServerDirectives()->MutableUpdateDatasyncDirective()->SetKey("directive_key");
        ahCtx.TestCtx().AddProtobufItem(response, SCENARIO_RPC_RESPONSE_ITEM_NAME, NAppHost::EContextItemKind::Input);

        const auto status = handler.Execute(ahCtx);
        UNIT_ASSERT(!status.Defined());

        const auto resultResponse = ahCtx.TestCtx().GetOnlyProtobufItem<NAlice::NRpc::TRpcResponseProto>(MM_RPC_RESPONSE_ITEM_NAME);

        auto expectedResult = JsonToProto<NAlice::NRpc::TRpcResponseProto>(JsonFromString(TStringBuf(R"({
            "server_directives": [
                {
                    "UpdateDatasyncDirective": {
                        "key": "directive_key"
                    }
                }
            ],
            "response_body": {
                "value": "payload",
                "@type": "type.googleapis.com\/google.protobuf.StringValue"
            },
            "analytics_info": {
                "analytics_info": {
                    "product_scenario_name": "psn"
                }
            }
        })")));
        expectedResult.SetVersion(NAlice::VERSION_STRING);

        UNIT_ASSERT_MESSAGES_EQUAL(resultResponse, expectedResult);
    }
}

} // namespace
