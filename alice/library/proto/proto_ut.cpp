#include "proto.h"

#include <alice/library/proto/ut/protos/test.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(ProtoBinary) {
    Y_UNIT_TEST(SerializeThenParse) {
        TFoo foo1;
        foo1.SetBar("hello");

        TFoo foo2 = ParseProto<TFoo>(foo1.SerializeAsString());
        UNIT_ASSERT_VALUES_EQUAL(foo1.GetBar(), foo2.GetBar());
    }

    Y_UNIT_TEST(ParseThenSerialize) {
        TFoo source;
        source.SetBar("hello");

        TString str1 = source.SerializeAsString();
        TString str2 = ParseProto<TFoo>(str1).SerializeAsString();
        UNIT_ASSERT_VALUES_EQUAL(str1, str2);
    }
}

Y_UNIT_TEST_SUITE(ParseProtoText) {
    Y_UNIT_TEST(Empty) {
        UNIT_ASSERT_NO_EXCEPTION(ParseProtoText<TFoo>(""));
    }

    Y_UNIT_TEST(SerializeThenParse) {
        TFoo foo1;
        foo1.SetBar("hello");

        TFoo foo2 = ParseProtoText<TFoo>(SerializeProtoText(foo1));
        UNIT_ASSERT_VALUES_EQUAL(foo1.GetBar(), foo2.GetBar());
    }

    Y_UNIT_TEST(ParseThenSerialize) {
        TFoo source;
        source.SetBar("hello");

        TString str1 = SerializeProtoText(source);
        TString str2 = SerializeProtoText(ParseProtoText<TFoo>(str1));
        UNIT_ASSERT_VALUES_EQUAL(str1, str2);
    }
}
