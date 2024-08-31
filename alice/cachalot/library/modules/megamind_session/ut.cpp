#include <alice/cachalot/library/modules/megamind_session/service.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NCachalotProtocol;
using namespace NCachalot;

Y_UNIT_TEST_SUITE(MegamindSessionService) {

Y_UNIT_TEST(KeyFromRequestFirst) {
    TMegamindSessionLoadRequest loadReq;
    loadReq.SetUuid("ffffffffffffffffffffffffff160039");
    loadReq.SetRequestId("0e6efa206ef04d70bdb9fd6eb2d85a41");

    TMegamindSessionRequest req;
    req.MutableLoadRequest()->Swap(&loadReq);

    TMegamindSessionKey key = ConstructKeyFromRequest(req);
    UNIT_ASSERT_VALUES_EQUAL(key.Key, "ffffffffffffffffffffffffff160039@0e6efa206ef04d70bdb9fd6eb2d85a41");
    UNIT_ASSERT_VALUES_EQUAL(key.ShardKey, 1783902118364140);
}

Y_UNIT_TEST(KeyFromRequestSecond) {
    TMegamindSessionLoadRequest loadReq;
    loadReq.SetUuid("ffffffffffffffffffffffffff841029");
    loadReq.SetDialogId("ee77eb59e1044029b8013d3391b18e9f");
    loadReq.SetRequestId("674523c7c5fe4df2b840692341d02a57");

    TMegamindSessionRequest req;
    req.MutableLoadRequest()->Swap(&loadReq);

    TMegamindSessionKey key = ConstructKeyFromRequest(req);
    UNIT_ASSERT_VALUES_EQUAL(key.Key, "ffffffffffffffffffffffffff841029$ee77eb59e1044029b8013d3391b18e9f@674523c7c5fe4df2b840692341d02a57");
    UNIT_ASSERT_VALUES_EQUAL(key.ShardKey, 1421437924292963);
}

Y_UNIT_TEST(KeyFromStoreRequest) {
    TMegamindSessionStoreRequest storeReq;
    storeReq.SetUuid("ffffffffffffffffffffffffff841029");
    storeReq.SetDialogId("ee77eb59e1044029b8013d3391b18e9f");
    storeReq.SetRequestId("674523c7c5fe4df2b840692341d02a57");
    storeReq.SetPuid("42");

    TMegamindSessionRequest req;
    req.MutableStoreRequest()->Swap(&storeReq);

    TMegamindSessionKey key = ConstructKeyFromRequest(req);
    UNIT_ASSERT_VALUES_EQUAL(key.Key, "ffffffffffffffffffffffffff841029$ee77eb59e1044029b8013d3391b18e9f@674523c7c5fe4df2b840692341d02a57");
    UNIT_ASSERT_VALUES_EQUAL(key.ShardKey, 1421437924292963);
}


};
