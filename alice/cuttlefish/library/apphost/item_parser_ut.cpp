#include "item_parser.h"

#include <alice/cuttlefish/library/apphost/test_proto/test.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NCuttlefish;

namespace {

NTest::TMessageWithString GetMessageWithString() {
    NTest::TMessageWithString message;
    message.SetSomeField("some_field");
    message.SetStr("some_str");

    return message;
}

} // namespace

class TCuttlefishAppHostItemParserTest : public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishAppHostItemParserTest);
    UNIT_TEST(TestParseProtobufItem);
    UNIT_TEST(TestParseProtobufItemException);
    UNIT_TEST(TestTryParseProtobufItem);
    UNIT_TEST_SUITE_END();

public:
    void TestParseProtobufItem() {
        const TString messageWithStringSerialization = GetMessageWithString().SerializeAsString();
        NAppHost::NService::TProtobufItem item(messageWithStringSerialization);

        {
            const auto result = ParseProtobufItem<NTest::TMessageWithString>(item);
            UNIT_ASSERT_VALUES_EQUAL(result.GetSomeField(), "some_field");
            UNIT_ASSERT_VALUES_EQUAL(result.GetStr(), "some_str");
        }

        {
            NTest::TMessageWithString result;
            ParseProtobufItem(item, result);
            UNIT_ASSERT_VALUES_EQUAL(result.GetSomeField(), "some_field");
            UNIT_ASSERT_VALUES_EQUAL(result.GetStr(), "some_str");
        }
    }

    void TestParseProtobufItemException() {
        const TString messageWithStringSerialization = GetMessageWithString().SerializeAsString();
        NAppHost::NService::TProtobufItem item(messageWithStringSerialization);

        {
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                ParseProtobufItem<NTest::TMessageWithSubMessage>(item),
                yexception,
                "Fail parsing 'NTest.TMessageWithSubMessage"
            );
        }

        {
            NTest::TMessageWithSubMessage result;
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                ParseProtobufItem(item, result),
                yexception,
                "Fail parsing 'NTest.TMessageWithSubMessage"
            );
            // Check that result partially filled
            UNIT_ASSERT_VALUES_EQUAL(result.GetSomeField(), "some_field");
        }
    }

    void TestTryParseProtobufItem() {
        const TString messageWithStringSerialization = GetMessageWithString().SerializeAsString();
        NAppHost::NService::TProtobufItem item(messageWithStringSerialization);

        {
            const auto result = TryParseProtobufItem<NTest::TMessageWithString>(item);
            UNIT_ASSERT(result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(result->GetSomeField(), "some_field");
            UNIT_ASSERT_VALUES_EQUAL(result->GetStr(), "some_str");
        }

        {
            const auto result = TryParseProtobufItem<NTest::TMessageWithSubMessage>(item);
            UNIT_ASSERT(!result.Defined());
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishAppHostItemParserTest);
