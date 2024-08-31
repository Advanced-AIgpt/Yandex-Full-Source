#include "proto_struct.h"

#include <alice/library/proto/ut/protos/test.pb.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {

const TStringBuf PROTO_STRING = R"(
    {
        "struct":{
            "key11":"value11",
            "key12":123,
            "key13":456.123,
            "key14":{
                "key21":"value21",
                "key22":889
            },
            "key15":{},
            "key16":[
                {},
                {"key261":"value261"},
                {"key262":"value262"},
                {"key263":"value263"}
            ],
            "key17":[
                {"common":{"val":650}},
                {"common":{"val":651}},
                {"common":{"val":652}}
            ],
            "key18":[
            ]
        }
    }
)";

Y_UNIT_TEST_SUITE(ProtobufStruct) {
    Y_UNIT_TEST(ProtobufStructSimple) {
        TTestStruct testStruct;
        const TProtoStructParser p;

        auto status = google::protobuf::util::JsonStringToMessage(PROTO_STRING.Data(), &testStruct);
        UNIT_ASSERT(status.ok());

        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key11", ""), "value11");
        UNIT_ASSERT_EQUAL(p.GetValueInt(testStruct.GetStruct(), "key12", 0), 123);
        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key12", "default"), "default");
        UNIT_ASSERT_EQUAL(p.GetValueDouble(testStruct.GetStruct(), "key13", 0.0), 456.123);

        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key14.key21", ""), "value21");
        UNIT_ASSERT_EQUAL(p.GetValueInt(testStruct.GetStruct(), "key14.key22", 0), 889);
        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key16.0.key261", "default"), "default");
        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key16.1.key261", ""), "value261");
        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key16.2.key262", ""), "value262");
        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key16.3.key263", ""), "value263");
        UNIT_ASSERT_STRINGS_EQUAL(p.GetValueString(testStruct.GetStruct(), "key16.3.key262", "default"), "default");
    } // Y_UNIT_TEST(ProtobufStructMain)

    Y_UNIT_TEST(ProtobufStructTypes) {
        TTestStruct testStruct;
        const TProtoStructParser p;

        auto status = google::protobuf::util::JsonStringToMessage(PROTO_STRING.Data(), &testStruct);
        UNIT_ASSERT(status.ok());

        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key00"), TProtoStructParser::EResult::Absent);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key00.key01"), TProtoStructParser::EResult::Absent);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key11"), TProtoStructParser::EResult::String);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key12"), TProtoStructParser::EResult::Numeric);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key13"), TProtoStructParser::EResult::Numeric);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key14"), TProtoStructParser::EResult::Map);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key14.key21"), TProtoStructParser::EResult::String);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key14.key22"), TProtoStructParser::EResult::Numeric);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key14.key23"), TProtoStructParser::EResult::Absent);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key15"), TProtoStructParser::EResult::Map);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key16"), TProtoStructParser::EResult::List);
        UNIT_ASSERT_EQUAL(p.TestKey(testStruct.GetStruct(), "key18"), TProtoStructParser::EResult::List);
    } // Y_UNIT_TEST(ProtobufStructTypes)

    Y_UNIT_TEST(ProtobufStructEnum) {
        TTestStruct testStruct;
        const TProtoStructParser p;

        auto status = google::protobuf::util::JsonStringToMessage(PROTO_STRING.Data(), &testStruct);
        UNIT_ASSERT(status.ok());

        bool rc = p.EnumerateKeys(testStruct.GetStruct(), "key16.[]", [p](const google::protobuf::Struct& ch) -> bool {
            if (ch.fields().empty()) {
                // Skip 1st item '{}'
                return false;
            }
            UNIT_ASSERT(p.GetValueString(ch, "key261") ||
                        p.GetValueString(ch, "key262") ||
                        p.GetValueString(ch, "key263"));
            return false;
        });
        UNIT_ASSERT(!rc);

        rc = p.EnumerateKeys(testStruct.GetStruct(), "key17.[].common", [p](const google::protobuf::Struct& ch) -> bool {
            return p.GetValueInt(ch, "val") == 651;
        });
        UNIT_ASSERT(rc);
    }
} // Y_UNIT_TEST_SUITE(ProtobufStructEnum)

} // namespace NAlice
