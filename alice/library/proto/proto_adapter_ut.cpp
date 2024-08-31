#include "proto_adapter.h"

#include <alice/library/json/json.h>
#include <alice/library/proto/ut/protos/test.pb.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {
const TStringBuf PROTO_STRING = R"(
    {
        "struct":{
            "key1":"StringValue",
            "key2":-123,
            "key3":456.123,
            "key4":178900,
            "key5":[
                {"k":1},
                {"k":2},
                {"k":3}
            ],
            "key6":[
            ],
            "key7": true,
            "key8": false
        }
    }
)";

Y_UNIT_TEST_SUITE(ProtoStructAdapter) {
    Y_UNIT_TEST(ProtoStructAdapterBasics) {
        TTestStruct testStruct;

        auto status = google::protobuf::util::JsonStringToMessage(PROTO_STRING.Data(), &testStruct);
        UNIT_ASSERT(status.ok());

        TProtoAdapter protoValue(testStruct.GetStruct());

        UNIT_ASSERT(protoValue.Has("key1"));
        UNIT_ASSERT(protoValue.Has("key2"));
        UNIT_ASSERT(protoValue.Has("key5"));
        UNIT_ASSERT(protoValue.Has("key6"));
        UNIT_ASSERT(!protoValue.Has("keyQ"));

        UNIT_ASSERT_EQUAL(protoValue["key1"].GetString(), "StringValue");
        UNIT_ASSERT_EQUAL(protoValue["key2"].GetDouble(), -123);
        UNIT_ASSERT_EQUAL(protoValue["key3"].GetDouble(), 456.123);
        UNIT_ASSERT_EQUAL(protoValue["key4"].GetUInteger(), 178900);

        int acc = 0;
        for (const auto &i : protoValue["key5"].GetArray()) {
            acc += i["k"].GetUInteger();
        }
        UNIT_ASSERT_EQUAL(acc, 6);

        UNIT_ASSERT(protoValue["key6"].GetArray().empty());
        UNIT_ASSERT(protoValue["key7"].GetBoolean() == true);
        UNIT_ASSERT(protoValue["key8"].GetBoolean() == false);
    }

    Y_UNIT_TEST(ProtoStructAdapterExceptionSafety) {
        TTestStruct testStruct;

        auto status = google::protobuf::util::JsonStringToMessage(PROTO_STRING.Data(), &testStruct);
        UNIT_ASSERT(status.ok());

        TProtoAdapter protoValue(testStruct.GetStruct());

        NJson::TJsonValue jsonValue = JsonFromString(PROTO_STRING)["struct"];

        // Check existing values are similar
        UNIT_ASSERT_EQUAL(protoValue["key1"].GetString(), jsonValue["key1"].GetString());
        UNIT_ASSERT_EQUAL(protoValue["key3"].GetDouble(), jsonValue["key3"].GetDouble());
        UNIT_ASSERT_EQUAL(protoValue["key4"].GetUInteger(), jsonValue["key4"].GetUInteger());
        UNIT_ASSERT_EQUAL(protoValue["key7"].GetBoolean(), jsonValue["key7"].GetBoolean());
        UNIT_ASSERT_EQUAL(protoValue["key5"].GetArray().size(), jsonValue["key5"].GetArray().size());

        UNIT_ASSERT(protoValue.IsExceptionSafeMode());
        
        // Check non-existing (default) values are similar
        UNIT_ASSERT_EQUAL(protoValue["key_faked"].GetString(), jsonValue["key_faked"].GetString());
        UNIT_ASSERT_EQUAL(protoValue["key_faked"].GetDouble(), jsonValue["key_faked"].GetDouble());
        UNIT_ASSERT_EQUAL(protoValue["key_faked"].GetUInteger(), jsonValue["key_faked"].GetUInteger());
        UNIT_ASSERT_EQUAL(protoValue["key_faked"].GetBoolean(), jsonValue["key_faked"].GetBoolean());
        UNIT_ASSERT_EQUAL(protoValue["key_faked"].GetArray().size(), jsonValue["key_faked"].GetArray().size());

        // Check values of incorrect type are similar
        UNIT_ASSERT_EQUAL(protoValue["key_1"].GetString(), jsonValue["key_1"].GetString());
        UNIT_ASSERT_EQUAL(protoValue["key_1"].GetDouble(), jsonValue["key_1"].GetDouble());
        UNIT_ASSERT_EQUAL(protoValue["key_1"].GetBoolean(), jsonValue["key_1"].GetBoolean());
        UNIT_ASSERT_EQUAL(protoValue["key_1"].GetArray().size(), jsonValue["key_1"].GetArray().size());

        // check unsafe mode
        protoValue.SetExceptionSafeMode(false);
        bool exceptionWasRaised = false;

        try {
            protoValue["key_faked"].GetString();
        } catch (const TProtoAdapterTypeException& e) {
            exceptionWasRaised = true;
        }

        UNIT_ASSERT(exceptionWasRaised);

        exceptionWasRaised = false;
        try {
            protoValue["key1"].GetUInteger();
        } catch (const TProtoAdapterTypeException& e) {
            exceptionWasRaised = true;
        }

        UNIT_ASSERT(exceptionWasRaised);
    }
}
}
