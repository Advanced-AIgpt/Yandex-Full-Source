#include <alice/hollywood/library/rpc_service/rpc_request.h>

#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/library/unittest/message_diff.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <contrib/libs/protobuf/src/google/protobuf/wrappers.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NRpc {

Y_UNIT_TEST_SUITE(RpcRequestWrapperTests) {
    Y_UNIT_TEST(TestConstruct) {
        TMockGlobalContext globalCtx;
        NAppHost::NService::TTestContext serviceCtx;

        NAlice::NScenarios::TRequestMeta meta;
        meta.SetRequestId("test_req_id");
        serviceCtx.AddProtobufItem(meta, REQUEST_META_ITEM_NAME, NAppHost::EContextItemKind::Input);

        NAliceProtocol::TContextLoadResponse clr;
        clr.SetUserTicket("test_user_ticket");
        serviceCtx.AddProtobufItem(clr, CONTEXT_LOAD_ITEM_NAME, NAppHost::EContextItemKind::Input);

        NAlice::NScenarios::TScenarioRpcRequest request;
        google::protobuf::StringValue payload;
        payload.set_value("test_payload");
        request.MutableRequest()->PackFrom(payload);
        request.MutableBaseRequest()->SetRandomSeed(12345);
        serviceCtx.AddProtobufItem(request, RPC_REQUEST_ITEM_NAME, NAppHost::EContextItemKind::Input);

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};
        TRpcRequestWrapper<google::protobuf::StringValue> requestWrapper{hwServiceCtx};
        UNIT_ASSERT_MESSAGES_EQUAL(*requestWrapper.GetRequestMeta(), meta);
        UNIT_ASSERT_MESSAGES_EQUAL(*requestWrapper.GetContextLoadResponse(), clr);
        UNIT_ASSERT_MESSAGES_EQUAL(requestWrapper.GetBaseRequest(), request.GetBaseRequest());
        UNIT_ASSERT_MESSAGES_EQUAL(requestWrapper.GetRequest(), payload);
    }
}

} // namespace NAlice::NHollywood::NRpc
