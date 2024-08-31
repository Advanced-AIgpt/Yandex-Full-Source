#include "protobuf.h"

#include <alice/library/proto/ut/protos/structs.pb.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;

Y_UNIT_TEST_SUITE(Protobuf) {
    Y_UNIT_TEST(TestSerializeObjectWithEnum_ZeroValue) {
        TTestMessageWithEnum msg{};
        msg.SetEnumField(static_cast<ETestEnum>(0));

        UNIT_ASSERT_MESSAGES_EQUAL(MessageToStructBuilder(msg).Build(), TProtoStructBuilder{}.Build());

        UNIT_ASSERT_MESSAGES_EQUAL(MessageToStructBuilder(msg, TBuilderOptions{.ProcessDefaultFields = true}).Build(),
                                   TProtoStructBuilder{}.Set("enum_field", "Zero").Build());
    }

    Y_UNIT_TEST(TestSerializeObjectWithEnum_NonZeroValue) {
        TTestMessageWithEnum msg{};
        msg.SetEnumField(ETestEnum::One);

        const auto actual = MessageToStructBuilder(msg).Build();
        const auto expected = TProtoStructBuilder{}.Set("enum_field", "One").Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestSerializeObjectWithEnum_PreferNumber) {
        TTestMessageWithEnum msg{};
        msg.SetEnumField(ETestEnum::One);

        const auto actual = MessageToStructBuilder(msg, TBuilderOptions{.PreferNumberInEnums = true}).Build();
        const auto expected = TProtoStructBuilder{}.SetInt("enum_field", 1).Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestSerializeObjectWithEnum_SkipUnknownValue) {
        TTestMessageWithEnum msg{};
        msg.SetEnumField(static_cast<ETestEnum>(255));

        const auto actual = MessageToStructBuilder(msg).Build();
        const auto expected = TProtoStructBuilder{}.Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestSerializeObjectWithEnum_RepeatedEnums) {
        TTestMessageWithRepeatedEnum msg{};
        msg.AddRepeatedEnumField(ETestEnum::One);
        msg.AddRepeatedEnumField(ETestEnum::Zero);
        msg.AddRepeatedEnumField(static_cast<ETestEnum>(255));

        const auto actual = MessageToStructBuilder(msg).Build();
        const auto expected = TProtoStructBuilder{}
                                  .Set("repeated_enum_field", TProtoListBuilder{}.Add("One").Add("Zero").Build())
                                  .Build();
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestBase64EncodeDecode) {
        TTestMessageWithRepeatedEnum msg{};
        msg.AddRepeatedEnumField(ETestEnum::One);
        msg.AddRepeatedEnumField(ETestEnum::Zero);
        msg.AddRepeatedEnumField(ETestEnum::One);
        msg.AddRepeatedEnumField(ETestEnum::One);
        msg.AddRepeatedEnumField(static_cast<ETestEnum>(255));

        TTestMessageWithRepeatedEnum actual{};
        ProtoFromBase64String(ProtoToBase64String(msg), actual);

        UNIT_ASSERT_MESSAGES_EQUAL(msg, actual);
    }

    Y_UNIT_TEST(TestBase64DecodeEncode) {
        const TString base64String = "CgYBAAEB/wE=";

        TTestMessageWithRepeatedEnum msg{};

        ProtoFromBase64String(base64String, msg);
        const auto actual = ProtoToBase64String(msg);

        UNIT_ASSERT_STRINGS_EQUAL(base64String, actual);
    }

    Y_UNIT_TEST(TestStructToProtoMessage) {
        TTestComplexMessage actual, expected;

        google::protobuf::Value valueField;
        valueField.set_string_value("value_field_value");

        const google::protobuf::Struct structField = TProtoStructBuilder{}.SetInt("struct_inner_field", 1).Build();

        const auto original = TProtoStructBuilder{}
            .Set("enum_field", "One")
            .Set("struct_field", structField)
            .Set("value_field", valueField)
            .Set("string_value_field", "string_value_field_value")
            .Set("repeated_enum_field", TProtoListBuilder{}.AddInt(1).Add("Zero").Build())
            .Set("inner_message",
                 TProtoStructBuilder{}
                     .Set("string_values",
                          TProtoListBuilder{}
                              .Add("1")
                              .Add("2")
                              .Build())
                     .Build())
            .Set("repeated_inner_messages",
                 TProtoListBuilder{}
                     .Add(TProtoStructBuilder{}
                              .Set("string_field", "0")
                              .Build())
                     .Add(TProtoStructBuilder{}
                              .SetDouble("double_field", 0.5)
                              .Build())
                     .Add(TProtoStructBuilder{}
                              .SetDouble("float_field", 1.5)
                              .Build())
                     .Add(TProtoStructBuilder{}
                              .SetInt("int32_field", -2)
                              .Build())
                     .Add(TProtoStructBuilder{}
                              .SetInt64("int64_field", -3)
                              .Build())
                     .Add(TProtoStructBuilder{}
                              .SetUInt("uint32_field", 4)
                              .Build())
                     .Add(TProtoStructBuilder{}
                              .SetUInt64("uint64_field", 5)
                              .Build())
                     .Add(TProtoStructBuilder{}
                              .SetBool("bool_field", true)
                              .Build())
                     .Build())
            .Build();

        expected.SetEnumField(ETestEnum::One);
        expected.MutableStructField()->CopyFrom(structField);
        expected.MutableValueField()->CopyFrom(valueField);
        expected.MutableStringValueField()->set_value("string_value_field_value");
        auto* expectedRepeatedEnumField = expected.MutableRepeatedEnumField();
        expectedRepeatedEnumField->Add(ETestEnum::One);
        expectedRepeatedEnumField->Add(ETestEnum::Zero);
        auto* expectedInnerMessage = expected.MutableInnerMessage();
        expectedInnerMessage->AddStringValues("1");
        expectedInnerMessage->AddStringValues("2");
        expected.AddRepeatedInnerMessages()->SetStringField("0");
        expected.AddRepeatedInnerMessages()->SetDoubleField(0.5);
        expected.AddRepeatedInnerMessages()->SetFloatField(1.5f);
        expected.AddRepeatedInnerMessages()->SetInt32Field(-2);
        expected.AddRepeatedInnerMessages()->SetInt64Field(-3l);
        expected.AddRepeatedInnerMessages()->SetUInt32Field(4u);
        expected.AddRepeatedInnerMessages()->SetUInt64Field(5ul);
        expected.AddRepeatedInnerMessages()->SetBoolField(true);

        StructToMessage(original, actual);

        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestThereAndBackAgain) {
        TTestComplexMessage actual, expected;

        google::protobuf::Value valueField;
        valueField.set_string_value("value_field_value");

        const google::protobuf::Struct structField = TProtoStructBuilder{}.SetInt("struct_inner_field", 1).Build();

        expected.SetEnumField(ETestEnum::One);
        expected.MutableStructField()->CopyFrom(structField);
        expected.MutableValueField()->CopyFrom(valueField);
        expected.MutableStringValueField()->set_value("string_value_field_value");
        auto* expectedRepeatedEnumField = expected.MutableRepeatedEnumField();
        expectedRepeatedEnumField->Add(ETestEnum::One);
        expectedRepeatedEnumField->Add(ETestEnum::Zero);
        auto* expectedInnerMessage = expected.MutableInnerMessage();
        expectedInnerMessage->AddStringValues("1");
        expectedInnerMessage->AddStringValues("2");
        expected.AddRepeatedInnerMessages()->SetStringField("0");
        expected.AddRepeatedInnerMessages()->SetDoubleField(0.5);
        expected.AddRepeatedInnerMessages()->SetFloatField(1.5f);
        expected.AddRepeatedInnerMessages()->SetInt32Field(-2);
        expected.AddRepeatedInnerMessages()->SetInt64Field(-3l);
        expected.AddRepeatedInnerMessages()->SetUInt32Field(4u);
        expected.AddRepeatedInnerMessages()->SetUInt64Field(5ul);
        expected.AddRepeatedInnerMessages()->SetBoolField(true);

        const auto& converted = MessageToStruct(expected);

        StructToMessage(converted, actual);

        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestAllMessageWrappers) {
        TTestMessageWithAllWrappers originalMessage;

        originalMessage.MutableStringValue()->set_value("string_value");
        originalMessage.MutableBytesValue()->set_value("bytes_value");
        originalMessage.MutableInt32Value()->set_value(1);
        originalMessage.MutableUInt32Value()->set_value(2);
        originalMessage.MutableInt64Value()->set_value(3);
        originalMessage.MutableUInt64Value()->set_value(4);
        originalMessage.MutableFloatValue()->set_value(5.5);
        originalMessage.MutableDoubleValue()->set_value(6.5);
        originalMessage.MutableBoolValue()->set_value(true);

        originalMessage.AddRepeatedStringValue()->set_value("repeated_string_value");
        originalMessage.AddRepeatedBytesValue()->set_value("repeated_bytes_value");
        originalMessage.AddRepeatedInt32Value()->set_value(7);
        originalMessage.AddRepeatedUInt32Value()->set_value(8);
        originalMessage.AddRepeatedInt64Value()->set_value(9);
        originalMessage.AddRepeatedUInt64Value()->set_value(10);
        originalMessage.AddRepeatedFloatValue()->set_value(11.5);
        originalMessage.AddRepeatedDoubleValue()->set_value(12.5);
        originalMessage.AddRepeatedBoolValue()->set_value(false);

        google::protobuf::Struct expectedStruct = TProtoStructBuilder{}
            .Set("string_value", "string_value")
            .Set("bytes_value", "bytes_value")
            .SetInt("int32_value", 1)
            .SetUInt("uint32_value", 2)
            .SetInt64("int64_value", 3)
            .SetUInt64("uint64_value", 4)
            .SetFloat("float_value", 5.5)
            .SetDouble("double_value", 6.5)
            .SetBool("bool_value", true)
            .Set("repeated_string_value",
                 TProtoListBuilder{}
                     .Add("repeated_string_value")
                 .Build())
            .Set("repeated_bytes_value",
                 TProtoListBuilder{}
                     .Add("repeated_bytes_value")
                 .Build())
            .Set("repeated_int32_value",
                 TProtoListBuilder{}
                     .AddInt(7)
                 .Build())
            .Set("repeated_uint32_value",
                 TProtoListBuilder{}
                     .AddUInt(8)
                 .Build())
            .Set("repeated_int64_value",
                 TProtoListBuilder{}
                     .AddInt64(9)
                 .Build())
            .Set("repeated_uint64_value",
                 TProtoListBuilder{}
                     .AddUInt64(10)
                 .Build())
            .Set("repeated_float_value",
                 TProtoListBuilder{}
                     .AddFloat(11.5)
                 .Build())
            .Set("repeated_double_value",
                 TProtoListBuilder{}
                     .AddDouble(12.5)
                 .Build())
            .Set("repeated_bool_value",
                 TProtoListBuilder{}
                     .AddBool(false)
                 .Build())
            .Build();

        google::protobuf::Struct actualStruct = MessageToStruct(originalMessage);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedStruct, actualStruct);

        // Test that we can convert struct back
        TTestMessageWithAllWrappers parsedMessage = StructToMessage<TTestMessageWithAllWrappers>(actualStruct);
        UNIT_ASSERT_MESSAGES_EQUAL(originalMessage, parsedMessage);
    }

    Y_UNIT_TEST(TestMapStructs) {
        TTestMessageWithMaps message;

        (*message.MutableStrings())["Strings_1_key"] = "strings_1_value";
        (*message.MutableStrings())["Strings_2_key"] = "strings_2_value";

        (*message.MutableBools())["Bools_key"] = true;
        (*message.MutableDoubles())["Doubles_key"] = 1.23;
        (*message.MutableEnums())["Enums_key"] = ETestEnum::One;
        (*message.MutableFloats())["Floats_key"] = 2.34;
        (*message.MutableInts32())["Ints32_key"] = -1;
        (*message.MutableInts64())["Ints64_key"] = -2;
        (*message.MutableUints32())["Uints32_key"] = 1;
        (*message.MutableUints64())["Uints64_key"] = 2;
        (*message.MutableStringValues())["StringValues_key"].set_value("StringValues_value");
        (*message.MutableBytesValues())["BytesValues_key"].set_value("BytesValues_value");
        (*message.MutableInt32Values())["Int32Values_key"].set_value(-11);
        (*message.MutableUInt32Values())["UInt32Values_key"].set_value(11);
        (*message.MutableInt64Values())["Int64Values_key"].set_value(-22);
        (*message.MutableUInt64Values())["UInt64Values_key"].set_value(22);
        (*message.MutableFloatValues())["FloatValues_key"].set_value(12.34);
        (*message.MutableDoubleValues())["DoubleValues_key"].set_value(123.4);
        (*message.MutableBoolValues())["BoolValues_key"].set_value(true);

        TTestMessageWithMaps::TTestInnerMessage innerMessage;
        (*innerMessage.MutableInnerStrings())["inner_strings_key_1"] = "inner_strings_value_1";
        (*innerMessage.MutableInnerStrings())["inner_strings_key_2"] = "inner_strings_value_2";
        innerMessage.SetInnerInt(12345);
        (*message.MutableTestInnerMessages())["TestInnerMessages_key"] = innerMessage;

        (*message.MutableListedMap())[1] = "listed_value_1";

        const auto resultStruct = MessageToStruct(message);

        const auto expectedStruct = TProtoStructBuilder{}
            .Set("strings", TProtoStructBuilder{}
                                .Set("Strings_1_key", "strings_1_value")
                                .Set("Strings_2_key", "strings_2_value")
                                .Build())
            .Set("bools", TProtoStructBuilder{}.SetBool("Bools_key", true).Build())
            .Set("doubles", TProtoStructBuilder{}.SetDouble("Doubles_key", 1.23).Build())
            .Set("enums", TProtoStructBuilder{}.Set("Enums_key", "One").Build())
            .Set("floats", TProtoStructBuilder{}.SetDouble("Floats_key", 2.34f).Build())
            .Set("ints32", TProtoStructBuilder{}.SetInt("Ints32_key", -1).Build())
            .Set("ints64", TProtoStructBuilder{}.SetInt64("Ints64_key", -2).Build())
            .Set("uints32", TProtoStructBuilder{}.SetUInt("Uints32_key", 1).Build())
            .Set("uints64", TProtoStructBuilder{}.SetUInt64("Uints64_key", 2).Build())
            .Set("string_values", TProtoStructBuilder{}.Set("StringValues_key", "StringValues_value").Build())
            .Set("bytes_values", TProtoStructBuilder{}.Set("BytesValues_key", "BytesValues_value").Build())
            .Set("int32_values", TProtoStructBuilder{}.SetInt("Int32Values_key", -11).Build())
            .Set("int64_values", TProtoStructBuilder{}.SetInt64("Int64Values_key", -22).Build())
            .Set("uint32_values", TProtoStructBuilder{}.SetUInt("UInt32Values_key", 11).Build())
            .Set("uint64_values", TProtoStructBuilder{}.SetUInt64("UInt64Values_key", 22).Build())
            .Set("float_values", TProtoStructBuilder{}.SetDouble("FloatValues_key", 12.34f).Build())
            .Set("double_values", TProtoStructBuilder{}.SetDouble("DoubleValues_key", 123.4).Build())
            .Set("bool_values", TProtoStructBuilder{}.SetBool("BoolValues_key", true).Build())
            .Set("test_inner_messages", TProtoStructBuilder{}
                                            .Set("TestInnerMessages_key",
                                                 TProtoStructBuilder{}
                                                    .SetInt("inner_int", 12345)
                                                    .Set("inner_strings", TProtoStructBuilder{}
                                                                              .Set("inner_strings_key_1", "inner_strings_value_1")
                                                                              .Set("inner_strings_key_2", "inner_strings_value_2")
                                                                              .Build())
                                                    .Build())
                                            .Build())
            .Set("listed_map", TProtoListBuilder{}
                                   .Add(TProtoStructBuilder{}
                                            .SetInt("key", 1)
                                            .Set("value", "listed_value_1")
                                            .Build())
                                   .Build())
            .Build();

        UNIT_ASSERT_MESSAGES_EQUAL(expectedStruct, resultStruct);

        TTestMessageWithMaps resultMessage;
        StructToMessage(resultStruct, resultMessage);
        UNIT_ASSERT_MESSAGES_EQUAL(message, resultMessage);
    }
}

} // namespace
