#include "asr.h"

#include <voicetech/library/proto_api/yaldi.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NAsrAdapter;


class TAsrProtoCensorAsrTest: public TTestBase {
    UNIT_TEST_SUITE(TAsrProtoCensorAsrTest);
    UNIT_TEST(TestGetCensoredYaldiInitRequestReturnsInitRequestWithNoContactBookItemsWhenPassedInitRequestWithContactBook);
    UNIT_TEST_SUITE_END();

public:
    void TestGetCensoredYaldiInitRequestReturnsInitRequestWithNoContactBookItemsWhenPassedInitRequestWithContactBook() {
        YaldiProtobuf::InitRequest testRequest;
        testRequest.mutable_user_info()->AddContactBookItems()->SetDisplayName("user 1");
        testRequest.mutable_user_info()->AddContactBookItems()->SetDisplayName("user 2");

        auto censoredRequest = GetCensoredYaldiInitRequest(testRequest);
        UNIT_ASSERT_EQUAL(censoredRequest.user_info().ContactBookItemsSize(), 0);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TAsrProtoCensorAsrTest);
