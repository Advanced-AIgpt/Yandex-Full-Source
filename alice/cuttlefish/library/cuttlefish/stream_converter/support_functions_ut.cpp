#include "support_functions.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NCuttlefish::NAppHostServices::NSupport;
using namespace NVoicetech::NUniproxy2;
using namespace NJson;

namespace {
    static const TString MESSAGE_WITH_HEADER = R"(
{
  "event": {
    "header": {
        "messageId": "id"
    },
    "payload": {
    }
  }
}
)";
    static const TString MESSAGE_WITHOUT_HEADER = R"(
{
  "event": {
    "payload": {
    }
  }
}
)";

}


class TCuttlefishAppHostServicesSupportFunctionsTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishAppHostServicesSupportFunctionsTest);
    UNIT_TEST(TestGetHeaderOrThrow);
    UNIT_TEST(TestGetJsonValueByPathOrThrow);
    UNIT_TEST_SUITE_END();

public:
    void TestGetHeaderOrThrow() {
        {
            TMessage message(TMessage::FromClient, MESSAGE_WITH_HEADER);

            UNIT_ASSERT_NO_EXCEPTION(GetHeaderOrThrow(message));
            UNIT_ASSERT_VALUES_EQUAL(GetHeaderOrThrow(message).MessageId, "id");
        }

        {
            TMessage message(TMessage::FromClient, MESSAGE_WITHOUT_HEADER);

            UNIT_ASSERT_EXCEPTION_CONTAINS(GetHeaderOrThrow(message), yexception, "header not found");
        }
    }

    void TestGetJsonValueByPathOrThrow() {
        NJson::TJsonValue json = NJson::TJsonMap();
        json["key"]["subkey"] = "value1";

        UNIT_ASSERT_NO_EXCEPTION(GetJsonValueByPathOrThrow(json, TStringBuf("key.subkey")));
        UNIT_ASSERT_VALUES_EQUAL(GetJsonValueByPathOrThrow(json, TStringBuf("key.subkey")), "value1");

        UNIT_ASSERT_EXCEPTION_CONTAINS(GetJsonValueByPathOrThrow(json, TStringBuf("random_key.key")), yexception, "random_key.key not found in json");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishAppHostServicesSupportFunctionsTest)
