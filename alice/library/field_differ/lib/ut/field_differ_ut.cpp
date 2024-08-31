#include <alice/library/field_differ/lib/ut/testing.pb.h>

#include <alice/library/field_differ/lib/field_differ.h>

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {

Y_UNIT_TEST_SUITE(FieldDiffer) {
    Y_UNIT_TEST(WithDiffDouble) {
        TTestingMessage message1;
        message1.SetDoubleValue(1337);
        TTestingMessage message2;
        message2.SetDoubleValue(7331);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("DoubleValue");
            diff.SetFirstValue("1337");
            diff.SetSecondValue("7331");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffInt64) {
        TTestingMessage message1;
        message1.SetImportantInt64Value(1337);
        TTestingMessage message2;
        message2.SetImportantInt64Value(7331);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("ImportantInt64Value");
            diff.SetFirstValue("1337");
            diff.SetSecondValue("7331");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithoutDiffInt64) {
        TTestingMessage message1;
        message1.SetNotImportantInt64Value(1337);
        TTestingMessage message2;
        message2.SetNotImportantInt64Value(7331);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffDoubles) {
        TTestingMessage message1;
        message1.AddDoubleValues(1);
        message1.AddDoubleValues(2);
        message1.AddDoubleValues(3);
        TTestingMessage message2;
        message2.AddDoubleValues(1);
        message2.AddDoubleValues(2);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("DoubleValues");
            diff.SetFirstValue("[1,2,3]");
            diff.SetSecondValue("[1,2]");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffDoublesDifferentValues) {
        TTestingMessage message1;
        message1.AddDoubleValues(1);
        message1.AddDoubleValues(2);
        TTestingMessage message2;
        message2.AddDoubleValues(2);
        message2.AddDoubleValues(1);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("DoubleValues");
            diff.SetFirstValue("[1,2]");
            diff.SetSecondValue("[2,1]");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffEnum) {
        TTestingMessage message1;
        message1.SetEnumValue(TTestingMessage::First);
        TTestingMessage message2;
        message2.SetEnumValue(TTestingMessage::Second);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("EnumValue");
            diff.SetFirstValue("First");
            diff.SetSecondValue("Second");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffEnums) {
        TTestingMessage message1;
        message1.AddEnumValues(TTestingMessage::First);
        message1.AddEnumValues(TTestingMessage::Second);
        message1.AddEnumValues(TTestingMessage::First);
        TTestingMessage message2;
        message2.AddEnumValues(TTestingMessage::First);
        message2.AddEnumValues(TTestingMessage::Second);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("EnumValues");
            diff.SetFirstValue(R"(["First","Second","First"])");
            diff.SetSecondValue(R"(["First","Second"])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffEnumsDifferentValues) {
        TTestingMessage message1;
        message1.AddEnumValues(TTestingMessage::Second);
        message1.AddEnumValues(TTestingMessage::First);
        TTestingMessage message2;
        message2.AddEnumValues(TTestingMessage::First);
        message2.AddEnumValues(TTestingMessage::Second);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("EnumValues");
            diff.SetFirstValue(R"(["Second","First"])");
            diff.SetSecondValue(R"(["First","Second"])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffMessage) {
        TTestingMessage message1;
        message1.MutableInnerMessage()->SetStringValue("lol");
        TTestingMessage message2;
        message2.MutableInnerMessage()->SetStringValue("kek");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("InnerMessage");
            diff.SetFirstValue(R"({"StringValue":"lol"})");
            diff.SetSecondValue(R"({"StringValue":"kek"})");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffMessages) {
        TTestingMessage message1;
        message1.AddInnerMessages()->SetStringValue("3");
        message1.AddInnerMessages()->SetStringValue("2");
        message1.AddInnerMessages()->SetStringValue("1");

        TTestingMessage message2;
        message2.AddInnerMessages()->SetStringValue("3");
        message2.AddInnerMessages()->SetStringValue("2");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("InnerMessages");
            diff.SetFirstValue(R"([{"StringValue":"3"},{"StringValue":"2"},{"StringValue":"1"}])");
            diff.SetSecondValue(R"([{"StringValue":"3"},{"StringValue":"2"}])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffMessagesDifferentValues) {
        TTestingMessage message1;
        message1.AddInnerMessages()->SetStringValue("1");
        message1.AddInnerMessages()->SetStringValue("2");

        TTestingMessage message2;
        message2.AddInnerMessages()->SetStringValue("2");
        message2.AddInnerMessages()->SetStringValue("1");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("InnerMessages");
            diff.SetFirstValue(R"([{"StringValue":"1"},{"StringValue":"2"}])");
            diff.SetSecondValue(R"([{"StringValue":"2"},{"StringValue":"1"}])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffStrings) {
        TTestingMessage message1;
        message1.AddStringValues("lol");
        message1.AddStringValues("kek");
        message1.AddStringValues("lol");
        TTestingMessage message2;
        message2.AddStringValues("lol");
        message2.AddStringValues("kek");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("StringValues");
            diff.SetFirstValue(R"(["lol","kek","lol"])");
            diff.SetSecondValue(R"(["lol","kek"])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffStringsDifferentValues) {
        TTestingMessage message1;
        message1.AddStringValues("lol");
        message1.AddStringValues("kek");
        TTestingMessage message2;
        message2.AddStringValues("kek");
        message2.AddStringValues("lol");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("StringValues");
            diff.SetFirstValue(R"(["lol","kek"])");
            diff.SetSecondValue(R"(["kek","lol"])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffInnerMessage) {
        TTestingMessage message1;
        message1.MutableInnerMessage2()->MutableInnerMessage3()->SetStringValue("lol");
        TTestingMessage message2;
        message2.MutableInnerMessage2()->MutableInnerMessage3()->SetStringValue("kek");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("InnerMessage2.InnerMessage3.StringValue");
            diff.SetFirstValue("lol");
            diff.SetSecondValue("kek");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffInnerMessages) {
        TTestingMessage message1;
        message1.AddInnerMessages2()->MutableInnerMessage3()->SetStringValue("lol");
        TTestingMessage message2;
        message2.AddInnerMessages2()->MutableInnerMessage3()->SetStringValue("kek");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("InnerMessages2.InnerMessage3.StringValue");
            diff.SetFirstValue("lol");
            diff.SetSecondValue("kek");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffInnerMessagesDifferentLen) {
        TTestingMessage message1;
        message1.AddInnerMessages2()->MutableInnerMessage3()->SetStringValue("lol");
        message1.AddInnerMessages2()->MutableInnerMessage3()->SetStringValue("lol");
        TTestingMessage message2;
        message2.AddInnerMessages2()->MutableInnerMessage3()->SetStringValue("kek");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("InnerMessages2");
            diff.SetFirstValue(R"([{"InnerMessage3":{"StringValue":"lol"}},{"InnerMessage3":{"StringValue":"lol"}}])");
            diff.SetSecondValue(R"([{"InnerMessage3":{"StringValue":"kek"}}])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithoutDiffInnerMessages) {
        TTestingMessage message1;
        message1.AddInnerMessages2()->MutableInnerMessage3()->SetInt64Value(1337);
        TTestingMessage message2;
        message2.AddInnerMessages2()->MutableInnerMessage3()->SetInt64Value(7331);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithoutDiffInnerMessagesDifferentLen) {
        TTestingMessage message1;
        message1.AddInnerMessages2()->MutableInnerMessage3()->SetInt64Value(1337);
        message1.AddInnerMessages2()->MutableInnerMessage3()->SetInt64Value(7331);
        TTestingMessage message2;
        message2.AddInnerMessages2()->MutableInnerMessage3()->SetInt64Value(7331);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("InnerMessages2");
            diff.SetFirstValue(R"([{"InnerMessage3":{"Int64Value":"1337"}},{"InnerMessage3":{"Int64Value":"7331"}}])");
            diff.SetSecondValue(R"([{"InnerMessage3":{"Int64Value":"7331"}}])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffMap) {
        TTestingMessage message1;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("lol");
            (*message1.MutableMapValue())["lol"] = innerMessage3;
        }

        TTestingMessage message2;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("kek");
            (*message2.MutableMapValue())["lol"] = innerMessage3;
        }

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("MapValue.value.StringValue");
            diff.SetFirstValue("lol");
            diff.SetSecondValue("kek");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffMapTwoKeys) {
        TTestingMessage message1;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("lol");
            (*message1.MutableMapValue())["lol"] = innerMessage3;
            (*message1.MutableMapValue())["kek"] = innerMessage3;
        }

        TTestingMessage message2;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("lol");
            (*message2.MutableMapValue())["lol"] = innerMessage3;
        }

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected1;
        {
            auto& diff = *expected1.AddDiffs();
            diff.SetPath("MapValue");
            diff.SetFirstValue(
                R"([{"key":"kek","value":{"StringValue":"lol"}},{"key":"lol","value":{"StringValue":"lol"}}])");
            diff.SetSecondValue(R"([{"key":"lol","value":{"StringValue":"lol"}}])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        TDifferReport expected2;
        {
            auto& diff = *expected2.AddDiffs();
            diff.SetPath("MapValue");
            diff.SetFirstValue(
                R"([{"key":"lol","value":{"StringValue":"lol"}},{"key":"kek","value":{"StringValue":"lol"}}])");
            diff.SetSecondValue(R"([{"key":"lol","value":{"StringValue":"lol"}}])");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_DIFF);
        }
        UNIT_ASSERT(google::protobuf::util::MessageDifferencer::Equals(expected1, differReport) ||
                    google::protobuf::util::MessageDifferencer::Equals(expected2, differReport));
    }

    Y_UNIT_TEST(WithoutDiffMapTwoKeys) {
        TTestingMessage message1;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("lol");
            (*message1.MutableMapValue())["lol"] = innerMessage3;
            (*message1.MutableMapValue())["kek"] = innerMessage3;
        }

        TTestingMessage message2;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("lol");
            (*message2.MutableMapValue())["kek"] = innerMessage3;
            (*message2.MutableMapValue())["lol"] = innerMessage3;
        }

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithDiffMapTwoDifferentKeys) {
        TTestingMessage message1;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("lol");
            (*message1.MutableMapValue())["lol"] = innerMessage3;
            (*message1.MutableMapValue())["kek"] = innerMessage3;
        }

        TTestingMessage message2;
        {
            TTestingMessage::TInnerMessage2::TInnerMessage3 innerMessage3;
            innerMessage3.SetStringValue("lol");
            (*message2.MutableMapValue())["lol"] = innerMessage3;
            (*message2.MutableMapValue())["lol2"] = innerMessage3;
        }

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);

        TDifferReport expected;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(CheckSizes) {
        TFieldDiffer fieldDiffer;
        {
            TTestingMessage message;
            fieldDiffer.ScanDescriptor(*message.GetDescriptor());
        }
        const auto expectedTraverseFieldsSize = fieldDiffer.TraverseFields.size();
        {
            TTestingMessage message;
            fieldDiffer.ScanDescriptor(*message.GetDescriptor());
        }
        UNIT_ASSERT_EQUAL_C(expectedTraverseFieldsSize, fieldDiffer.TraverseFields.size(),
                            ToString(fieldDiffer.TraverseFields.size()));
    }

    Y_UNIT_TEST(WithPresenceDouble) {
        TTestingMessage message1;
        message1.SetPresenceDoubleValue(1337);
        TTestingMessage message2;
        message2.SetPresenceDoubleValue(7331);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithPresenceDoubleFail) {
        TTestingMessage message1;
        message1.SetPresenceDoubleValue(1337);
        TTestingMessage message2;

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("PresenceDoubleValue");
            diff.SetFirstValue("1337");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_PRESENCE);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithPresenceEnum) {
        TTestingMessage message1;
        message1.SetPresenceEnumValue(TTestingMessage::First);
        TTestingMessage message2;
        message2.SetPresenceEnumValue(TTestingMessage::Second);

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithPresenceEnumFail) {
        TTestingMessage message1;
        message1.SetPresenceEnumValue(TTestingMessage::First);
        TTestingMessage message2;

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("PresenceEnumValue");
            diff.SetFirstValue("First");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_PRESENCE);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithPresenceInnerMessage) {
        TTestingMessage message1;
        message1.MutablePresenceInnerMessage()->SetStringValue("lol");
        TTestingMessage message2;
        message2.MutablePresenceInnerMessage()->SetStringValue("kek");

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }

    Y_UNIT_TEST(WithPresenceInnerMessageFail) {
        TTestingMessage message1;
        message1.MutablePresenceInnerMessage()->SetStringValue("lol");
        TTestingMessage message2;

        TFieldDiffer fieldDiffer;
        const auto differReport = fieldDiffer.FindDiffs(message1, message2);
        TDifferReport expected;
        {
            auto& diff = *expected.AddDiffs();
            diff.SetPath("PresenceInnerMessage");
            diff.SetFirstValue(R"({"StringValue":"lol"})");
            diff.SetImportantFieldCheck(EImportantFieldCheck::IFC_PRESENCE);
        }
        UNIT_ASSERT_MESSAGES_EQUAL(expected, differReport);
    }
}

} // namespace NAlice
