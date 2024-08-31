#include <alice/nlg/library/runtime_api/value.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>

using namespace NAlice::NNlg;

Y_UNIT_TEST_SUITE(NlgValue) {
    Y_UNIT_TEST(Constructors) {
        UNIT_ASSERT(TValue::Undefined().IsUndefined());
        UNIT_ASSERT(TValue::Bool(true).IsBool());
        UNIT_ASSERT(TValue::Integer(123).IsInteger());
        UNIT_ASSERT(TValue::Double(1.23).IsDouble());
        UNIT_ASSERT(TValue::String("abc").IsString());
        UNIT_ASSERT(TValue::List().IsList());
        UNIT_ASSERT(TValue::List({TValue::None(), TValue::Integer(1)}).IsList());
        UNIT_ASSERT(TValue::Dict().IsDict());
        UNIT_ASSERT(TValue::Dict({
                                     {"foo", TValue::Double(3.14)},
                                     {"bar", TValue::Bool(false)},
                                 })
                        .IsDict());
        UNIT_ASSERT(TValue::None().IsNone());
    }

    Y_UNIT_TEST(Equality) {
        UNIT_ASSERT(TValue::Undefined() == TValue::Undefined());

        UNIT_ASSERT(TValue::Bool(true) == TValue::Bool(true));
        UNIT_ASSERT(TValue::Bool(true) != TValue::Bool(false));
        UNIT_ASSERT(!(TValue::Bool(true) == TValue::Bool(false))); // paranoia runs deep

        UNIT_ASSERT(TValue::Integer(123) == TValue::Integer(123));
        UNIT_ASSERT(TValue::Integer(123) != TValue::Integer(124));

        UNIT_ASSERT(TValue::Double(123) == TValue::Double(123));
        UNIT_ASSERT(TValue::Double(123) != TValue::Double(124));

        UNIT_ASSERT(TValue::String("foo") == TValue::String("foo"));
        UNIT_ASSERT(TValue::String("foo") != TValue::String("bar"));

        UNIT_ASSERT(TValue::List({TValue::Bool(true), TValue::Integer(123), TValue::None()}) ==
                    TValue::List({TValue::Bool(true), TValue::Integer(123), TValue::None()}));
        UNIT_ASSERT(TValue::List({TValue::Bool(true), TValue::Integer(123), TValue::None()}) !=
                    TValue::List({TValue::Bool(true), TValue::Integer(321), TValue::None()}));

        UNIT_ASSERT(TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::Integer(2)}}) ==
                    TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::Integer(2)}}));
        UNIT_ASSERT(TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::Integer(2)}}) !=
                    TValue::Dict({{"foo", TValue::Integer(1)}, {"baz", TValue::Integer(2)}}));
        UNIT_ASSERT(TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::Integer(2)}}) !=
                    TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::Integer(3)}}));

        UNIT_ASSERT(TValue::None() == TValue::None());

        UNIT_ASSERT(TValue::None() != TValue::Integer(123));
    }

    Y_UNIT_TEST(TruthValue) {
        UNIT_ASSERT(!TruthValue(TValue::Undefined()));
        UNIT_ASSERT(!TruthValue(TValue::Bool(false)));
        UNIT_ASSERT(TruthValue(TValue::Bool(true)));
        UNIT_ASSERT(!TruthValue(TValue::Integer(0)));
        UNIT_ASSERT(TruthValue(TValue::Integer(-1)));
        UNIT_ASSERT(!TruthValue(TValue::Double(0)));
        UNIT_ASSERT(TruthValue(TValue::Double(3.14)));
        UNIT_ASSERT(!TruthValue(TValue::String("")));
        UNIT_ASSERT(TruthValue(TValue::String("asdf")));
        UNIT_ASSERT(!TruthValue(TValue::List()));
        UNIT_ASSERT(TruthValue(TValue::List({TValue::None()})));
        UNIT_ASSERT(!TruthValue(TValue::Dict()));
        UNIT_ASSERT(TruthValue(TValue::Dict({{"foo", TValue::None()}})));
        UNIT_ASSERT(!TruthValue(TValue::Range({5, 5, 1})));
        UNIT_ASSERT(TruthValue(TValue::Range({0, 5, 1})));
        UNIT_ASSERT(!TruthValue(TValue::None()));
    }

    Y_UNIT_TEST(Json) {
        UNIT_ASSERT(TValue::ParseJson("true") == TValue::Bool(true));
        UNIT_ASSERT(TValue::ParseJson("1") == TValue::Integer(1));
        UNIT_ASSERT(TValue::ParseJson("\"\\n\"") == TValue::String("\n"));

        UNIT_ASSERT(TValue::ParseJson("[]") == TValue::List());
        UNIT_ASSERT(TValue::ParseJson("{}") == TValue::Dict());

        UNIT_ASSERT_VALUES_EQUAL(TValue::ParseJson("{\"foo\": [{\"bar\": {}}, 3.14], \"bar\": 123}"),
                                 TValue::Dict({
                                     {"foo", TValue::List({
                                                 TValue::Dict({{"bar", TValue::Dict()}}),
                                                 TValue::Double(3.14),
                                             })},
                                     {"bar", TValue::Integer(123)},
                                 }));

        // plainly invalid JSON
        UNIT_ASSERT_EXCEPTION(TValue::ParseJson("{{}}"), TJsonSyntaxError);

        // incomplete JSON
        UNIT_ASSERT_EXCEPTION(TValue::ParseJson("{"), TJsonSyntaxError);

        // integer value would overflow i64 (the only integer type TValue supports)
        UNIT_ASSERT_EXCEPTION(TValue::ParseJson("18446744073709551615"), TJsonIntegerError);

        // no sneaking in multiple values
        UNIT_ASSERT_EXCEPTION(TValue::ParseJson("1\n2\n"), TJsonSyntaxError);
        UNIT_ASSERT_EXCEPTION(TValue::ParseJson("{\"a\": 1}//d{\"a\": 2}\n"), TJsonSyntaxError);
    }

    Y_UNIT_TEST(JsonValue) {
        TStringBuf jsons[] = {
            "{}", "12.3", "4", "null", "true", "[1, 2, {}]", "{\"foo\": [null]}",
        };

        for (auto json : jsons) {
            TMemoryInput in(json);
            auto parsed = NJson::ReadJsonTree(&in, /* throwOnError = */ true);
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::ParseJson(json), TValue::FromJsonValue(parsed), "json = " << json);
        }
    }

    Y_UNIT_TEST(AsJson) {
        const TStringBuf jsons[] = {
            "{}", "12.3", "4", "null", "true", "[1,2,{}]", "{\"foo\":[null]}", "{\"hello\":[1,2,3]}",
        };

        for (const auto json : jsons) {
            const auto value = TValue::ParseJson(json);
            const TString serializedValue = (TStringBuilder{} << value.AsJson());
            UNIT_ASSERT_VALUES_EQUAL_C(json, serializedValue, "json = " << json);
        }
    }

    Y_UNIT_TEST(AsJsonUndefined) {
        auto value = TValue::ParseJson("{\"hello\": [1, 2]}");
        value.GetMutableDict()["hello"].GetMutableList()[1] = TValue::Undefined();

        UNIT_ASSERT_EXCEPTION((TStringBuilder{} << value.AsJson()), TValueError);
        UNIT_ASSERT_VALUES_EQUAL("{\"hello\":[1,\"<undefined>\"]}",
                                 (TString{TStringBuilder{} << value.AsJson(/* printUndefined= */ true)}));
    }

    Y_UNIT_TEST(GetAttrLoad) {
        auto v0 = TValue::Integer(1);
        UNIT_ASSERT(GetAttrLoad(v0, "a").IsUndefined());

        auto v1 = TValue::Dict({{"b", TValue::None()}});
        UNIT_ASSERT(GetAttrLoad(v1, "a").IsUndefined());

        auto v2 = TValue::Dict({{"a", TValue::None()}});
        UNIT_ASSERT(GetAttrLoad(v2, "a").IsNone());
    }

    Y_UNIT_TEST(Output) {
        auto toString = [](const TValue& value) {
            TString result;
            TStringOutput out(result);
            out << value;
            return result;
        };

        UNIT_ASSERT_VALUES_EQUAL("", toString(TValue::Undefined()));
        UNIT_ASSERT_VALUES_EQUAL("True", toString(TValue::Bool(true)));
        UNIT_ASSERT_VALUES_EQUAL("False", toString(TValue::Bool(false)));
        UNIT_ASSERT_VALUES_EQUAL("-1234", toString(TValue::Integer(-1234)));
        UNIT_ASSERT_VALUES_EQUAL("-1.1", toString(TValue::Double(-1.1)));
        UNIT_ASSERT_VALUES_EQUAL("asdf", toString(TValue::String("asdf")));

        UNIT_ASSERT_VALUES_EQUAL(
            "[1, 2, '3']", toString(TValue::List({TValue::Integer(1), TValue::Integer(2), TValue::String("3")})));
        UNIT_ASSERT_VALUES_EQUAL("{'foo': 1, 'bar': 'baz'}", toString(TValue::Dict({{"foo", TValue::Integer(1)},
                                                                                    {"bar", TValue::String("baz")}})));

        UNIT_ASSERT_VALUES_EQUAL("xrange(5)", toString(TValue::Range({0, 5, 1})));
        UNIT_ASSERT_VALUES_EQUAL("xrange(2, 5)", toString(TValue::Range({2, 5, 1})));
        UNIT_ASSERT_VALUES_EQUAL("xrange(2, 5, 2)", toString(TValue::Range({2, 5, 2})));
        UNIT_ASSERT_VALUES_EQUAL("xrange(0, 5, 2)", toString(TValue::Range({0, 5, 2})));

        UNIT_ASSERT_VALUES_EQUAL("None", toString(TValue::None()));
    }

    Y_UNIT_TEST(GetItemLoad) {
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoadInt(TValue::String("asdf"), 123));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoadInt(TValue::Integer(123), 123));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoadInt(TValue::Dict({{{"123", TValue::None()}}}), 123));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoadInt(TValue::List({TValue::None()}), 123));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(true),
                                 GetItemLoadInt(TValue::List({TValue::None(), TValue::Bool(true)}), 1));

        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoad(TValue::String("asdf"), TValue::Integer(123)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoad(TValue::Integer(123), TValue::Integer(123)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::None(),
                                 GetItemLoad(TValue::Dict({{{"123", TValue::None()}}}), TValue::String("123")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(),
                                 GetItemLoad(TValue::Dict({{{"123", TValue::None()}}}), TValue::Integer(123)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(),
                                 GetItemLoad(TValue::List({TValue::None()}), TValue::Integer(123)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(),
                                 GetItemLoad(TValue::List({TValue::None(), TValue::Bool(true)}), TValue::String("1")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::None(),
                                 GetItemLoad(TValue::List({TValue::None(), TValue::Bool(true)}), TValue::Integer(-2)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(true),
                                 GetItemLoad(TValue::List({TValue::None(), TValue::Bool(true)}), TValue::Integer(-1)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::None(),
                                 GetItemLoad(TValue::List({TValue::None(), TValue::Bool(true)}), TValue::Integer(0)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(true),
                                 GetItemLoad(TValue::List({TValue::None(), TValue::Bool(true)}), TValue::Integer(1)));

        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(7), GetItemLoad(TValue::Range({1, 9, 3}), TValue::Integer(-1)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(1), GetItemLoad(TValue::Range({1, 9, 3}), TValue::Integer(0)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(4), GetItemLoad(TValue::Range({1, 9, 3}), TValue::Integer(1)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(7), GetItemLoad(TValue::Range({1, 9, 3}), TValue::Integer(2)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoad(TValue::Range({1, 9, 3}), TValue::Integer(3)));

        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoad(TValue::String("asdf"), TValue::String("123")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), GetItemLoad(TValue::List({TValue::None(), TValue::Bool(true)}),
                                                                  TValue::String("123")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::None(),
                                 GetItemLoad(TValue::Dict({{{"123", TValue::None()}}}), TValue::String("123")));
    }

    Y_UNIT_TEST(Iterator) {
        auto toList = [](const TValue& value) -> TValue {
            TValue::TList result;
            for (const auto& item : value) {
                result.push_back(item);
            }
            return TValue::List(std::move(result));
        };

        auto undefined = TValue::Undefined();
        auto list = TValue::List({{TValue::Integer(123), TValue::Bool(false)}});
        auto dict = TValue::Dict({{{"foo", TValue::Integer(123)}, {"bar", TValue::Bool(true)}}});
        auto range = TValue::Range({0, 3, 1});

        UNIT_ASSERT_VALUES_EQUAL(TValue::List(), toList(undefined));
        UNIT_ASSERT_VALUES_EQUAL(list, toList(list));
        UNIT_ASSERT_VALUES_EQUAL(TValue::List({{TValue::String("foo"), TValue::String("bar")}}), toList(dict));
        UNIT_ASSERT_VALUES_EQUAL(TValue::List({{TValue::Integer(0), TValue::Integer(1), TValue::Integer(2)}}),
                                 toList(range));
    }

    Y_UNIT_TEST(Freeze) {
        auto dict = TValue::Dict();
        GetAttrStore(dict, "foo") = TValue::Dict();
        auto& nestedDict = GetAttrStore(dict, "foo");

        UNIT_ASSERT(!dict.IsFrozen());
        UNIT_ASSERT(!nestedDict.IsFrozen());

        dict.Freeze();

        UNIT_ASSERT(dict.IsFrozen());
        UNIT_ASSERT(nestedDict.IsFrozen());

        UNIT_ASSERT_EXCEPTION(GetAttrStore(dict, "bar") = TValue::Integer(2), TValueFrozenError);
        UNIT_ASSERT_EXCEPTION(GetAttrStore(nestedDict, "bar") = TValue::Integer(2), TValueFrozenError);
    }

    Y_UNIT_TEST(ToInteger) {
        const std::pair<TValue, i64> positiveCases[] = {
            {TValue::Bool(false), 0},
            {TValue::Bool(true), 1},
            {TValue::Integer(0), 0},
            {TValue::Integer(5), 5},
            {TValue::Integer(-1), -1},
        };

        for (const auto& [value, expected] : positiveCases) {
            const auto maybeInteger = ToInteger(value);
            UNIT_ASSERT(maybeInteger.Defined());
            UNIT_ASSERT_VALUES_EQUAL_C(expected, *maybeInteger, "value = " << Repr(value));
        }

        const TValue negativeCases[] = {
            TValue::Undefined(),
            TValue::Double(5),
            TValue::String(TStringBuf("foo")),
            TValue::List(),
            TValue::Dict(),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (const auto& value : negativeCases) {
            UNIT_ASSERT_C(!ToInteger(value).Defined(), "value = " << Repr(value));
        }
    }

    Y_UNIT_TEST(ToDouble) {
        const std::pair<TValue, double> positiveCases[] = {
            {TValue::Bool(false), 0},
            {TValue::Bool(true), 1},
            {TValue::Integer(0), 0},
            {TValue::Integer(5), 5},
            {TValue::Integer(-1), -1},
            {TValue::Double(0), 0},
            {TValue::Double(5), 5},
            {TValue::Double(-1), -1},
            {TValue::Double(3.14), 3.14},
        };

        for (const auto& [value, expected] : positiveCases) {
            const auto maybeDouble = ToDouble(value);
            UNIT_ASSERT(maybeDouble.Defined());
            UNIT_ASSERT_VALUES_EQUAL_C(expected, *maybeDouble, "value = " << Repr(value));
        }

        const TValue negativeCases[] = {
            TValue::Undefined(),
            TValue::String(TStringBuf("foo")),
            TValue::List(),
            TValue::Dict(),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (const auto& value : negativeCases) {
            UNIT_ASSERT_C(!ToDouble(value).Defined(), "value = " << Repr(value));
        }
    }
}
