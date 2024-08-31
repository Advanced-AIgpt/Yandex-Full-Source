#include <alice/nlg/library/runtime/builtins.h>
#include <alice/library/sys_datetime/sys_datetime.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <contrib/libs/double-conversion/double-conversion/double-conversion.h>
#include <util/generic/hash_set.h>

using namespace NAlice;
using namespace NAlice::NNlg;
using namespace NAlice::NNlg::NBuiltins;

namespace {

constexpr TStringBuf RuLanguage = "ru";

TValue Str(const TStringBuf x) {
    return TValue::String(x);
}

TValue SeqToList(const TVector<int> seq) {
    TVector<TValue> res;
    for (auto el : seq) {
        res.emplace_back(TValue::Integer(el));
    }
    return TValue::List(res);
}

NJson::TJsonValue ReadJson(TStringBuf raw) {
    NJson::TJsonValue val;
    NJson::ReadJsonTree(raw, &val, true);
    return val;
}

}  // namespace

Y_UNIT_TEST_SUITE(TestBuiltins) {
    Y_UNIT_TEST(Attr) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        auto dict = TValue::Dict({{"foo", TValue::Integer(123)}});
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(123),
                                 Attr(ctx, /* globals = */ nullptr, dict, TValue::String("foo")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Undefined(), Attr(ctx, /* globals = */ nullptr, dict, TValue::String("bar")));

        UNIT_ASSERT_EXCEPTION(Attr(ctx, /* globals = */ nullptr, dict, TValue::Integer(123)),
                              NAlice::NNlg::TTypeError);
    }

    Y_UNIT_TEST(Random) {
        const TEnvironment env;

        i64 randomValue = 0;
        TFakeRng rng(TFakeRng::TIntegerTag{}, [&randomValue]() { return randomValue++; });
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        auto string = TValue::String("asdf");
        auto list = TValue::List({TValue::Integer(1), TValue::Integer(2), TValue::Integer(3)});
        auto range = TValue::Range({0, 5, 2});

        auto sample = [&ctx](const TValue& value) {
            THashSet<TValue> samples;
            for (size_t i = 0; i < 4; ++i) {
                samples.insert(Random(ctx, /* globals = */ nullptr, value));
            }
            return samples;
        };

        UNIT_ASSERT_VALUES_EQUAL(
            THashSet<TValue>({TValue::String('a'), TValue::String('s'), TValue::String('d'), TValue::String('f')}),
            sample(string));
        UNIT_ASSERT_VALUES_EQUAL(THashSet<TValue>({TValue::Integer(1), TValue::Integer(2), TValue::Integer(3)}),
                                 sample(list));
        UNIT_ASSERT_VALUES_EQUAL(THashSet<TValue>({TValue::Integer(0), TValue::Integer(2), TValue::Integer(4)}),
                                 sample(range));

        UNIT_ASSERT_EXCEPTION(sample(TValue::Integer(3)), TTypeError);
    }

    Y_UNIT_TEST(TrimWithEllipsis) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        auto trim = [&ctx](TString&& str, i64 width) {
            return TrimWithEllipsis(ctx, /* globals = */ nullptr, TValue::String(std::move(str)),
                                    TValue::Integer(width));
        };

        UNIT_ASSERT_VALUES_EQUAL(TValue::String(""), trim("", 0));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String(""), trim("", 10));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("двадцатичетырёхбуквенное!"), trim("двадцатичетырёхбуквенное!", 20));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("двадцатичетырёхбуквенное"), trim("двадцатичетырёхбуквенное", 20));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("..."), trim("привет как дела", 0));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет..."), trim("привет как дела", 1));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет..."), trim("привет как дела", 3));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет..."), trim("привет как дела", 6));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет как..."), trim("привет как дела", 7));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет как дела"), trim("привет как дела", 13));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет как дела"), trim("привет как дела", 100));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет..."), trim("привет. как? дела!", 3));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет. как..."), trim("привет. как? дела!", 9));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("привет. как? дела!"), trim("привет. как? дела!", 100));
    }

    Y_UNIT_TEST(OnlyTextOnlyVoice) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        constexpr auto TEXT = TText::TFlags{} | TText::EFlag::Text;
        constexpr auto VOICE = TText::TFlags{} | TText::EFlag::Voice;

        TText text;
        text.Append("text", TEXT);
        text.Append("voice", VOICE);
        text.Append("both", TEXT | VOICE);

        TText expectedText{"textboth", TEXT};
        UNIT_ASSERT_VALUES_EQUAL(TValue::String(expectedText),
                                 OnlyText(ctx, /* globals = */ nullptr, TValue::String(text)));

        TText expectedVoice{"voiceboth", VOICE};
        UNIT_ASSERT_VALUES_EQUAL(TValue::String(expectedVoice),
                                 OnlyVoice(ctx, /* globals = */ nullptr, TValue::String(text)));
    }

    Y_UNIT_TEST(Float) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, TValue, double> positiveCases[] = {
            {TValue::Undefined(), TValue::Double(1.14), 1.14},
            {TValue::Bool(false), TValue::Double(1.0), 0.0},
            {TValue::Integer(2), TValue::Double(1.0), 2.0},
            {TValue::Double(3.14), TValue::Double(1.0), 3.14},
            {TValue::String("3.14"), TValue::Double(1.0), 3.14},
            {TValue::List(), TValue::Integer(2), 2.0},
        };

        for (const auto& [target, defValue, expected] : positiveCases) {
            const auto actual = Float(ctx, /* globals = */ nullptr, target, defValue);
            UNIT_ASSERT_C(actual.IsDouble(),
                          "target = " << Repr(target) << ", defValue = " << Repr(defValue));
            UNIT_ASSERT_VALUES_EQUAL_C(expected, actual.GetDouble(),
                                       "target = " << Repr(target) << ", defValue = " << Repr(defValue));
        }

        const TValue negativeDefs[] = {
            TValue::Undefined(),
            TValue::String(""),
        };

        for (const auto& defValue : negativeDefs) {
            UNIT_ASSERT_EXCEPTION_C(Float(ctx, /* globals = */ nullptr, TValue::Double(0), defValue),
                                    TTypeError, "defValue = " << Repr(defValue));
        }
    }

    Y_UNIT_TEST(Round) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, i64, double, TStringBuf> positiveCases[] = {
            {TValue::Double(0), 10, 0.0, "common"},
            {TValue::Double(1), 10, 1.0, "common"},
            {TValue::Double(3.14), 0, 3.0, "common"},
            {TValue::Double(3.14), 1, 3.1, "common"},
            {TValue::Double(3.14), 2, 3.14, "common"},
            {TValue::Double(3.14), 3, 3.14, "common"},
            {TValue::Double(3.14), 0, 3.0, "floor"},
            {TValue::Double(3.14), 0, 4.0, "ceil"},
            {TValue::Integer(5), 0, 5.0, "common"},
            {TValue::Integer(5), 0, 5.0, "floor"},
            {TValue::Integer(5), 0, 5.0, "ceil"},
            {TValue::Double(5.5), 0, 6, "common"},
            {TValue::Double(1234.5678), 0, 1235.0, "common"},
            {TValue::Double(1234.5678), 2, 1234.57, "common"},
            {TValue::Double(1234.5678), -2, 1200.0, "common"},
            {TValue::Double(1234.5678), -3, 1000.0, "common"},
            {TValue::Double(1234.5678), -4, 0.0, "common"},
            {TValue::Double(901551301.59814453125), 7, 901551301.5981445, "common"},
        };

        auto allDigits = [](const TValue& value) -> TString {
            const auto maybeDoubleValue = ToDouble(value);
            UNIT_ASSERT_C(maybeDoubleValue, "value = " << Repr(value));

            constexpr size_t bufferSize = 400;
            char buffer[bufferSize];
            double_conversion::StringBuilder builder(buffer, bufferSize);
            double_conversion::DoubleToStringConverter converter(double_conversion::DoubleToStringConverter::NO_FLAGS,
                                                                 "inf", "nan", 'e',
                                                                 Min<int>(), Max<int>(), Max<int>(), Max<int>());
            UNIT_ASSERT_C(converter.ToShortest(*maybeDoubleValue, &builder), "value = " << Repr(value));
            return TString{buffer, static_cast<size_t>(builder.position())};
        };

        for (const auto& [target, precision, expected, method] : positiveCases) {
            TVector<TValue> methods;
            methods.push_back(TValue::String(method));
            if (method == "common") {
                methods.push_back(TValue::None());
            }

            for (const auto& currentMethod : methods) {
                const auto actual = Round(ctx, /* globals = */ nullptr,
                                          target, TValue::Integer(precision), currentMethod);
                UNIT_ASSERT_VALUES_EQUAL_C(actual, TValue::Double(expected),
                                           "actual = " << allDigits(actual) << ", target = " << allDigits(target)
                                           << ", precision = " << precision << ", method = " << currentMethod);
            }
        }

        UNIT_ASSERT_EXCEPTION(Round(ctx, /* globals = */ nullptr,
                              TValue::String("3"), TValue::Integer(5), TValue::String("common")), TTypeError);
        UNIT_ASSERT_EXCEPTION(Round(ctx, /* globals = */ nullptr,
                              TValue::Double(3), TValue::Double(0), TValue::String("common")), TTypeError);
        UNIT_ASSERT_EXCEPTION(Round(ctx, /* globals = */ nullptr,
                              TValue::Double(3), TValue::Integer(5000), TValue::String("common")), TValueError);
        UNIT_ASSERT_EXCEPTION(Round(ctx, /* globals = */ nullptr,
                              TValue::Double(3), TValue::Integer(0), TValue::String("uncommon")), TValueError);
    }

    Y_UNIT_TEST(Int) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, TValue, i64> positiveCases[] = {
            {TValue::Undefined(), TValue::Integer(3), 3},
            {TValue::Bool(false), TValue::Integer(3), 0},
            {TValue::Integer(2), TValue::Integer(1), 2},
            {TValue::Double(3.14), TValue::Integer(1), 3},
            {TValue::String("3"), TValue::Integer(1), 3},
            {TValue::List(), TValue::Integer(2), 2},
        };

        for (const auto& [target, defValue, expected] : positiveCases) {
            const auto actual = Int(ctx, /* globals = */ nullptr, target, defValue);
            UNIT_ASSERT_C(actual.IsInteger(),
                          "target = " << Repr(target) << ", defValue = " << Repr(defValue));
            UNIT_ASSERT_VALUES_EQUAL_C(expected, actual.GetInteger(),
                                       "target = " << Repr(target) << ", defValue = " << Repr(defValue));
        }

        const TValue negativeDefs[] = {
            TValue::Undefined(),
            TValue::String(""),
        };

        for (const auto& defValue : negativeDefs) {
            UNIT_ASSERT_EXCEPTION_C(Int(ctx, /* globals = */ nullptr, TValue::Integer(0), defValue),
                                    TTypeError, "defValue = " << Repr(defValue));
        }
    }

    Y_UNIT_TEST(Inflect) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT_VALUES_EQUAL(TValue::String("совой"), Inflect(ctx, /* globals = */ nullptr,
                                                                    TValue::String("сова"),
                                                                    TValue::String("sg,ablt"),
                                                                    TValue::Bool(false)));

        UNIT_ASSERT_VALUES_EQUAL(TValue::String("Брюса Ли"), Inflect(ctx, /* globals = */ nullptr,
                                                                            TValue::String("Брюс Ли"),
                                                                            TValue::String("sg,gent"),
                                                                            TValue::Bool(true)));

        UNIT_ASSERT_EXCEPTION(Inflect(ctx, /* globals = */ nullptr,
                                      TValue::Integer(123), TValue::String("sg,gent"), TValue::Bool(false)),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(Inflect(ctx, /* globals = */ nullptr,
                                      TValue::String("сова"), TValue::Integer(123), TValue::Bool(false)),
                              TTypeError);
    }

    Y_UNIT_TEST(Default) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, TValue, bool, TValue> positiveCases[] = {
            {TValue::Undefined(), TValue::Integer(123), false, TValue::Integer(123)},
            {TValue::Integer(0), TValue::Integer(123), false, TValue::Integer(0)},
            {TValue::Integer(0), TValue::Integer(123), true, TValue::Integer(123)},
        };

        for (const auto& [target, defValue, boolean, expected] : positiveCases) {
            const auto actual = Default(ctx, /* globals = */ nullptr, target, defValue, TValue::Bool(boolean));
            UNIT_ASSERT_VALUES_EQUAL_C(expected, actual,
                                       "target = " << Repr(target) << ", defValue = " << Repr(defValue)
                                                   << ", boolean = " << boolean);
        }
    }

    Y_UNIT_TEST(HtmlEscape) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        THashMap<char, TStringBuf> seqs = {
            {'&', "&amp;"},
            {'"', "&quot;"},
            {'\'', "&apos;"},
            {'>', "&gt;"},
            {'<', "&lt;"},
            {'\\', "&#92;"},
            {'\n', "<br/>"},
        };

        for (unsigned char c = 0; c < 128; ++c) {
            const TString target(1, c);

            TString expected = target;
            if (const auto* seq = seqs.FindPtr(c)) {
                expected = TString{*seq};
            }

            const auto actual = HtmlEscape(ctx, /* globals = */ nullptr, TValue::String(target));
            UNIT_ASSERT_VALUES_EQUAL_C(actual, TValue::String(expected), "c = " << c);
        }

        UNIT_ASSERT_VALUES_EQUAL(HtmlEscape(ctx, /* globals = */ nullptr, TValue::String("фы>ва")),
                                 TValue::String("фы&gt;ва"));

        UNIT_ASSERT_EXCEPTION(HtmlEscape(ctx, /* globals = */ nullptr, TValue::None()), TTypeError);
    }

    Y_UNIT_TEST(CapitalizeFirst) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, TStringBuf> positiveCases[] = {
            {"", ""},
            {"П", "П"},
            {"п", "П"},
            {"превед медвед", "Превед медвед"},
            {"   превед медвед", "Превед медвед"},
            {"hello, world!", "Hello, world!"},
        };

        for (const auto& [target, expected] : positiveCases) {
            {
                const auto actual = CapitalizeFirst(ctx, /* globals = */ nullptr, TValue::String(target));
                UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(expected), actual, "target = " << target);
            }
            {
                const auto actual = Capitalize(ctx, /* globals = */ nullptr, TValue::String(target));
                UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(expected), actual, "target = " << target);
            }
        }

        UNIT_ASSERT_EXCEPTION(CapitalizeFirst(ctx, /* globals = */ nullptr, TValue::Integer(123)), TTypeError);
        UNIT_ASSERT_EXCEPTION(Capitalize(ctx, /* globals = */ nullptr, TValue::Integer(123)), TTypeError);
    }

    Y_UNIT_TEST(DecapitalizeFirst) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, TStringBuf> positiveCases[] = {
            {"", ""},
            {"П", "п"},
            {"п", "п"},
            {"Превед медвед", "превед медвед"},
            {"   Превед медвед", "превед медвед"},
            {"Hello, world!", "hello, world!"},
        };

        for (const auto& [target, expected] : positiveCases) {
            {
                const auto actual = DecapitalizeFirst(ctx, /* globals = */ nullptr, TValue::String(target));
                UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(expected), actual, "target = " << target);
            }
            {
                const auto actual = Decapitalize(ctx, /* globals = */ nullptr, TValue::String(target));
                UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(expected), actual, "target = " << target);
            }
        }

        UNIT_ASSERT_EXCEPTION(DecapitalizeFirst(ctx, /* globals = */ nullptr, TValue::Integer(123)), TTypeError);
        UNIT_ASSERT_EXCEPTION(Decapitalize(ctx, /* globals = */ nullptr, TValue::Integer(123)), TTypeError);
    }

    Y_UNIT_TEST(ToJson) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto expected = TStringBuf("{\"foo\":[1,2.3,\"4\",null,true,[5]]}");

        const auto target = TValue::Dict({{"foo", TValue::List({TValue::Integer(1),
                                                                TValue::Double(2.3),
                                                                TValue::String("4"),
                                                                TValue::None(),
                                                                TValue::Bool(true),
                                                                TValue::List({TValue::Integer(5)})})}});

        UNIT_ASSERT_VALUES_EQUAL(TValue::String(expected), ToJson(ctx, /* globals = */ nullptr, target));
        UNIT_ASSERT_EXCEPTION(ToJson(ctx, /* globals = */ nullptr, TValue::Undefined()), TValueError);
    }

    Y_UNIT_TEST(Urlencode) {
        const TEnvironment env;
        TRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, TStringBuf> positiveCases[] = {
            {"", ""},
            {"hello", "hello"},
            {"Hello, world 1234", "Hello%2C%20world%201234"},
            {"Привет", "%D0%9F%D1%80%D0%B8%D0%B2%D0%B5%D1%82"},  // check UTF-8 quotation
        };

        for (const auto& [target, expected] : positiveCases) {
            const auto actual = Urlencode(ctx, /* globals = */ nullptr, TValue::String(target));
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(expected), actual, "target = " << target);
        }
    }

    Y_UNIT_TEST(Randuniform) {
        const TEnvironment env;

        constexpr size_t numSamples = 100;
        constexpr double from = 2.72;
        constexpr double to = 3.14;

        int randomIndex = 0;
        TFakeRng rng(TFakeRng::TDoubleTag{},
                     [&randomIndex]() { return randomIndex++ / static_cast<double>(numSamples); });
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        for (size_t i = 0; i < numSamples; ++i) {
            auto sample = Randuniform(ctx, /* globals = */ nullptr, TValue::Double(from), TValue::Double(to));
            UNIT_ASSERT(sample.IsDouble());
            double sampleDouble = sample.GetDouble();
            UNIT_ASSERT(from <= sampleDouble && sampleDouble < to);
        }
    }

    Y_UNIT_TEST(TestTypeTratis) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT(!TruthValue(TestDefined(ctx, /* globals = */ nullptr, TValue::Undefined())));
        UNIT_ASSERT(TruthValue(TestUndefined(ctx, /* globals = */ nullptr, TValue::Undefined())));

        using V = TValue;
        const std::tuple<TValue, bool, bool, bool, bool, bool, bool, bool, bool> traitsCases[] = {
            // value,             defined, undefined, iterable, mapping, none, number, sequence, string
            {V::Undefined(), false, true, true, false, false, false, false, false},
            {V::Bool(false), true, false, false, false, false, false, false, false},
            {V::Integer(1), true, false, false, false, false, true, false, false},
            {V::Double(1), true, false, false, false, false, true, false, false},
            {V::String(""), true, false, false, false, false, false, false, true},
            {V::List(), true, false, true, false, false, false, true, false},
            {V::Dict(), true, false, true, true, false, false, false, false},
            {V::Range({0, 5, 1}), true, false, true, false, false, false, true, false},
            {V::None(), true, false, false, false, true, false, false, false},
        };
        for (const auto& [value, defined, undefined, iterable, mapping, none, number, sequence, string] :
             traitsCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(defined, TruthValue(TestDefined(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", defined = " << defined);
            UNIT_ASSERT_VALUES_EQUAL_C(undefined, TruthValue(TestUndefined(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", undefined = " << undefined);
            UNIT_ASSERT_VALUES_EQUAL_C(iterable, TruthValue(TestIterable(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", iterable = " << iterable);
            UNIT_ASSERT_VALUES_EQUAL_C(mapping, TruthValue(TestMapping(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", mapping = " << mapping);
            UNIT_ASSERT_VALUES_EQUAL_C(none, TruthValue(TestNone(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", none = " << none);
            UNIT_ASSERT_VALUES_EQUAL_C(number, TruthValue(TestNumber(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", number = " << number);
            UNIT_ASSERT_VALUES_EQUAL_C(sequence, TruthValue(TestSequence(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", sequence = " << sequence);
            UNIT_ASSERT_VALUES_EQUAL_C(string, TruthValue(TestString(ctx, /* globals = */ nullptr, value)),
                                       "value = " << Repr(value) << ", string = " << string);
        }
    }

    Y_UNIT_TEST(TestDivisibility) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, TValue, bool> divCases[] = {
            {TValue::Integer(5), TValue::Integer(5), true},   {TValue::Integer(10), TValue::Integer(5), true},
            {TValue::Integer(11), TValue::Integer(5), false}, {TValue::Integer(0), TValue::Integer(5), true},
            {TValue::Integer(5), TValue::Double(2.5), true},  {TValue::Integer(5), TValue::Double(2.6), false},
        };
        for (const auto& [target, num, expected] : divCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(expected,
                                       TruthValue(TestDivisibleBy(ctx, /* globals = */ nullptr, target, num)),
                                       "target = " << target << ", num = " << num << ", expected = " << expected);
        }
    }

    Y_UNIT_TEST(TestEvenOdd) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::pair<TValue, bool> evenCases[] = {
            {TValue::Integer(0), true},   {TValue::Integer(1), false}, {TValue::Integer(2), true},
            {TValue::Integer(-1), false}, {TValue::Integer(-2), true},
        };

        for (const auto& [target, even] : evenCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(even, TruthValue(TestEven(ctx, /* globals = */ nullptr, target)),
                                       "target = " << target << ", even = " << even);
            UNIT_ASSERT_VALUES_EQUAL_C(!even, TruthValue(TestOdd(ctx, /* globals = */ nullptr, target)),
                                       "target = " << target << ", even = " << even);
        }
    }

    Y_UNIT_TEST(TestComparisons) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto one = TValue::Integer(1);
        const auto two = TValue::Integer(2);

        // light tests for the wrappers, operators themselves are
        // tested more thoroughly elsewhere
        UNIT_ASSERT_C(!TruthValue(TestEq(ctx, /* globals */ nullptr, one, two)), "1 is eq(2)");
        UNIT_ASSERT_C(!TruthValue(TestGe(ctx, /* globals */ nullptr, one, two)), "1 is ge(2)");
        UNIT_ASSERT_C(!TruthValue(TestGt(ctx, /* globals */ nullptr, one, two)), "1 is gt(2)");
        UNIT_ASSERT_C(TruthValue(TestLe(ctx, /* globals */ nullptr, one, two)), "1 is le(2)");
        UNIT_ASSERT_C(TruthValue(TestLt(ctx, /* globals */ nullptr, one, two)), "1 is lt(2)");
        UNIT_ASSERT_C(TruthValue(TestNe(ctx, /* globals */ nullptr, one, two)), "1 is ne(2)");

        UNIT_ASSERT_C(TruthValue(TestEq(ctx, /* globals */ nullptr, one, one)), "1 is eq(1)");
        UNIT_ASSERT_C(TruthValue(TestGe(ctx, /* globals */ nullptr, one, one)), "1 is ge(1)");
        UNIT_ASSERT_C(!TruthValue(TestGt(ctx, /* globals */ nullptr, one, one)), "1 is gt(1)");
        UNIT_ASSERT_C(TruthValue(TestLe(ctx, /* globals */ nullptr, one, one)), "1 is le(1)");
        UNIT_ASSERT_C(!TruthValue(TestLt(ctx, /* globals */ nullptr, one, one)), "1 is lt(1)");
        UNIT_ASSERT_C(!TruthValue(TestNe(ctx, /* globals */ nullptr, one, one)), "1 is ne(1)");
    }

    Y_UNIT_TEST(TestLowerUpper) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, bool, bool> strLowerUpper[] = {
            {"1234", false, false},
            {"123ф", true, false},
            {"123Ф", false, true},
            {"фыВА", false, false},
        };
        for (const auto& [str, lower, upper] : strLowerUpper) {
            UNIT_ASSERT_VALUES_EQUAL_C(lower, TruthValue(TestLower(ctx, /* globals = */ nullptr, TValue::String(str))),
                                       "str = " << str << ", lower = " << lower);
            UNIT_ASSERT_VALUES_EQUAL_C(upper, TruthValue(TestUpper(ctx, /* globals = */ nullptr, TValue::String(str))),
                                       "str = " << str << ", upper = " << upper);
        }
    }

    Y_UNIT_TEST(DictMethods) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto dict = TValue::Dict({{"foo", TValue::Integer(123)}, {"bar", TValue::None()}});
        THashSet<TValue> keys = {{TValue::String("foo"), TValue::String("bar")}};
        THashSet<TValue> values = {{TValue::Integer(123), TValue::None()}};
        THashSet<TValue> items = {{TValue::List({{TValue::String("foo"), TValue::Integer(123)}}),
                                   TValue::List({{TValue::String("bar"), TValue::None()}})}};

        auto testMethod = [](const TValue& actual, const THashSet<TValue>& expectedSet) {
            const auto& actualList = actual.GetList();
            THashSet<TValue> actualSet(actualList.begin(), actualList.end());
            UNIT_ASSERT_VALUES_EQUAL(expectedSet, actualSet);
        };

        testMethod(DictKeys(ctx, /* globals = */ nullptr, dict), keys);
        testMethod(DictValues(ctx, /* globals = */ nullptr, dict), values);
        testMethod(DictItems(ctx, /* globals = */ nullptr, dict), items);

        UNIT_ASSERT_VALUES_EQUAL(DictGet(ctx, /* globals = */ nullptr, dict, TValue::String("foo"), TValue::None()),
                                 TValue::Integer(123));
        UNIT_ASSERT_VALUES_EQUAL(DictGet(ctx, /* globals = */ nullptr, dict, TValue::String("baz"), TValue::None()),
                                 TValue::None());
        UNIT_ASSERT_VALUES_EQUAL(DictGet(ctx, /* globals = */ nullptr,
                                         dict, TValue::String("baz"), TValue::Integer(666)),
                                 TValue::Integer(666));

        const auto otherDict = TValue::Dict({{"foo", TValue::Integer(456)}, {"baz", TValue::Double(3.14)}});
        UNIT_ASSERT_VALUES_EQUAL(DictUpdate(ctx, /* globals = */ nullptr, dict, otherDict), TValue::None());

        UNIT_ASSERT_VALUES_EQUAL(DictGet(ctx, /* globals = */ nullptr, dict, TValue::String("foo"), TValue::None()),
                                 TValue::Integer(456));
        UNIT_ASSERT_VALUES_EQUAL(DictGet(ctx, /* globals = */ nullptr, dict, TValue::String("baz"), TValue::None()),
                                 TValue::Double(3.14));
    }

    Y_UNIT_TEST(ListMethods) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto list = TValue::List({TValue::Integer(1), TValue::Integer(2)});
        UNIT_ASSERT_VALUES_EQUAL(ListAppend(ctx, /* globals = */ nullptr, list, TValue::Integer(3)), TValue::None());
        UNIT_ASSERT_VALUES_EQUAL(list, TValue::List({TValue::Integer(1), TValue::Integer(2), TValue::Integer(3)}));
    }

    Y_UNIT_TEST(StringMethods) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto trueValue = TValue::Bool(true);
        const auto falseValue = TValue::Bool(false);

        // str.endswith
        UNIT_ASSERT_VALUES_EQUAL(trueValue,
                                 StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("Превед")));
        UNIT_ASSERT_VALUES_EQUAL(trueValue, StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("вед")));
        UNIT_ASSERT_VALUES_EQUAL(trueValue, StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("")));
        UNIT_ASSERT_VALUES_EQUAL(falseValue, StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("Д")));
        UNIT_ASSERT_VALUES_EQUAL(trueValue, StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"),
                                                        TValue::List({Str("Д"), Str("д")})));
        UNIT_ASSERT_VALUES_EQUAL(
            trueValue, StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"),
                                   TValue::List({Str("д"), TValue::Integer(1)}))); // just like Python's str.join
        UNIT_ASSERT_VALUES_EQUAL(falseValue,
                                 StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"), TValue::List()));
        UNIT_ASSERT_EXCEPTION(StrEndsWith(ctx, /* globals = */ nullptr, TValue::Integer(1), TValue::List()),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"), TValue::Range({0, 5, 1})),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(
            StrEndsWith(ctx, /* globals = */ nullptr, Str("Превед"), TValue::List({TValue::Integer(1)})),
            TTypeError);

        // str.join
        UNIT_ASSERT_VALUES_EQUAL(Str("Превед и медвед"), StrJoin(ctx, /* globals = */ nullptr, Str(" и "),
                                                                   TValue::List({Str("Превед"), Str("медвед")})));
        UNIT_ASSERT_VALUES_EQUAL(Str("Преведмедвед"), StrJoin(ctx, /* globals = */ nullptr, Str(""),
                                                                TValue::List({Str("Превед"), Str("медвед")})));
        UNIT_ASSERT_VALUES_EQUAL(Str("Превед"),
                                 StrJoin(ctx, /* globals = */ nullptr, Str(" и "), TValue::List({Str("Превед")})));
        UNIT_ASSERT_VALUES_EQUAL(Str(""), StrJoin(ctx, /* globals = */ nullptr, Str(" и "), TValue::List()));
        UNIT_ASSERT_EXCEPTION(StrJoin(ctx, /* globals = */ nullptr, TValue::None(), TValue::List()), TTypeError);
        UNIT_ASSERT_EXCEPTION(StrJoin(ctx, /* globals = */ nullptr, Str(" и "), TValue::List({TValue::Integer(1)})),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(StrJoin(ctx, /* globals = */ nullptr, Str(" и "), TValue::Range({0, 5, 1})),
                              TTypeError);

        // str.lower
        UNIT_ASSERT_VALUES_EQUAL(Str("превед"), StrLower(ctx, /* globals = */ nullptr, Str("Превед")));
        UNIT_ASSERT_VALUES_EQUAL(Str(""), StrLower(ctx, /* globals = */ nullptr, Str("")));
        UNIT_ASSERT_EXCEPTION(StrLower(ctx, /* globals = */ nullptr, TValue::None()), TTypeError);

        // str.lstrip
        UNIT_ASSERT_VALUES_EQUAL(Str("Превед"),
                                 StrLstrip(ctx, /* globals = */ nullptr, Str("\n\n \rПревед"), TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(
            Str("\n\n \rПревед"),
            StrLstrip(ctx, /* globals = */ nullptr, Str("-*-\n\n \rПревед"), TValue::String("-*е")));
        UNIT_ASSERT_EXCEPTION(StrLstrip(ctx, /* globals = */ nullptr, TValue::None(), TValue::None()), TTypeError);
        UNIT_ASSERT_EXCEPTION(StrLstrip(ctx, /* globals = */ nullptr, Str("Превед"), TValue::Integer(0)),
                              TTypeError);

        // str.replace
        UNIT_ASSERT_VALUES_EQUAL(Str("Пр*в*д"),
                                 StrReplace(ctx, /* globals = */ nullptr, Str("Превед"), Str("е"), Str("*")));
        UNIT_ASSERT_VALUES_EQUAL(Str("Пр*д"),
                                 StrReplace(ctx, /* globals = */ nullptr, Str("Превед"), Str("еве"), Str("*")));
        UNIT_ASSERT_VALUES_EQUAL(Str("Првд"),
                                 StrReplace(ctx, /* globals = */ nullptr, Str("Превед"), Str("е"), Str("")));
        UNIT_ASSERT_VALUES_EQUAL(Str(" П р е в е д "),
                                 StrReplace(ctx, /* globals = */ nullptr, Str("Превед"), Str(""), Str(" ")));
        UNIT_ASSERT_VALUES_EQUAL(Str(" "),
                                 StrReplace(ctx, /* globals = */ nullptr, Str(""), Str(""), Str(" ")));
        UNIT_ASSERT_EXCEPTION(StrReplace(ctx, /* globals = */ nullptr, TValue::None(), Str(""), Str("")),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(StrReplace(ctx, /* globals = */ nullptr, Str(""), TValue::None(), Str("")),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(StrReplace(ctx, /* globals = */ nullptr, Str(""), Str(""), TValue::None()),
                              TTypeError);

        // str.rstrip
        UNIT_ASSERT_VALUES_EQUAL(Str("Превед"),
                                 StrRstrip(ctx, /* globals = */ nullptr, Str("Превед\n\n \t"), TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(
            Str("Превед\n\n \t"),
            StrRstrip(ctx, /* globals = */ nullptr, Str("Превед\n\n \t**-"), TValue::String("-*е")));
        UNIT_ASSERT_EXCEPTION(StrRstrip(ctx, /* globals = */ nullptr, TValue::None(), TValue::None()), TTypeError);
        UNIT_ASSERT_EXCEPTION(StrRstrip(ctx, /* globals = */ nullptr, Str("Превед"), TValue::Integer(0)),
                              TTypeError);

        // str.split
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::List({Str("о"), Str(""), Str("ло"), Str("ло")}),
            StrSplit(ctx, /* globals = */ nullptr, Str("о::ло:ло"), Str(":"), TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::List({Str("о"), Str("ло:ло")}),
            StrSplit(ctx, /* globals = */ nullptr, Str("о::ло:ло"), Str("::"), TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::List({Str("о"), Str(""), Str("ло:ло")}),
            StrSplit(ctx, /* globals = */ nullptr, Str("о::ло:ло"), Str(":"), TValue::Integer(2)));
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::List({Str("о"), Str("ло"), Str("ло")}),
            StrSplit(ctx, /* globals = */ nullptr, Str("о  ло\nло"), TValue::None(), TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::List({Str("о"), Str("ло\nло")}),
            StrSplit(ctx, /* globals = */ nullptr, Str("о  ло\nло"), TValue::None(), TValue::Integer(1)));
        UNIT_ASSERT_EXCEPTION(StrSplit(ctx, /* globals = */ nullptr, TValue::None(), TValue::None(), TValue::None()),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(StrSplit(ctx, /* globals = */ nullptr, Str(""), TValue::Integer(2), TValue::None()),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(StrSplit(ctx, /* globals = */ nullptr, Str(""), TValue::None(), Str("")),
                              TTypeError);

        // str.startswith
        UNIT_ASSERT_VALUES_EQUAL(trueValue,
                                 StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("Превед")));
        UNIT_ASSERT_VALUES_EQUAL(trueValue,
                                 StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("Пре")));
        UNIT_ASSERT_VALUES_EQUAL(trueValue, StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("")));
        UNIT_ASSERT_VALUES_EQUAL(falseValue, StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"), Str("п")));
        UNIT_ASSERT_VALUES_EQUAL(trueValue, StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"),
                                                          TValue::List({Str("п"), Str("П")})));
        UNIT_ASSERT_VALUES_EQUAL(
            trueValue, StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"),
                                     TValue::List({Str("П"), TValue::Integer(1)}))); // just like Python's str.join
        UNIT_ASSERT_VALUES_EQUAL(falseValue,
                                 StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"), TValue::List()));
        UNIT_ASSERT_EXCEPTION(StrStartsWith(ctx, /* globals = */ nullptr, TValue::Integer(1), TValue::List()),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"), TValue::Range({0, 5, 1})),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(
            StrStartsWith(ctx, /* globals = */ nullptr, Str("Превед"), TValue::List({TValue::Integer(1)})),
            TTypeError);

        // str.strip
        UNIT_ASSERT_VALUES_EQUAL(Str("Превед"),
                                 StrStrip(ctx, /* globals = */ nullptr, Str("\n\r\rПревед\n\n \t"), TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(
            Str("\n\r\rПревед\n\n \t"),
            StrStrip(ctx, /* globals = */ nullptr, Str("***\n\r\rПревед\n\n \t**-"), TValue::String("-*е")));
        UNIT_ASSERT_EXCEPTION(StrStrip(ctx, /* globals = */ nullptr, TValue::None(), TValue::None()), TTypeError);
        UNIT_ASSERT_EXCEPTION(StrStrip(ctx, /* globals = */ nullptr, Str("Превед"), TValue::Integer(0)), TTypeError);

        // str.upper
        UNIT_ASSERT_VALUES_EQUAL(Str("ПРЕВЕД"), StrUpper(ctx, /* globals = */ nullptr, Str("Превед")));
        UNIT_ASSERT_VALUES_EQUAL(Str(""), StrUpper(ctx, /* globals = */ nullptr, Str("")));
        UNIT_ASSERT_EXCEPTION(StrUpper(ctx, /* globals = */ nullptr, TValue::None()), TTypeError);
    }

    Y_UNIT_TEST(ClientActionDirective) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto expected = TValue::Dict({
            {"name", TValue::String("foo")},
            {"payload", TValue::Integer(1)},
            {"type", TValue::String("bar")},
            {"sub_name", TValue::String("sub_foo")},
        });
        const auto actual = ClientActionDirective(ctx, /* globals = */ nullptr,
                                                  TValue::String("foo"),
                                                  TValue::Integer(1),
                                                  TValue::String("bar"),
                                                  TValue::String("sub_foo"));
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ServerActionDirective) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto expected = TValue::Dict({
            {"name", TValue::String("foo")},
            {"payload", TValue::Integer(1)},
            {"type", TValue::String("bar")},
            {"ignore_answer", TValue::Bool(true)},
        });
        const auto actual = ServerActionDirective(ctx, /* globals = */ nullptr,
                                                  TValue::String("foo"),
                                                  TValue::Integer(1),
                                                  TValue::String("bar"),
                                                  TValue::Bool(true));
        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Abs) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, TValue> positiveCases[] = {
            {TValue::Bool(true), TValue::Integer(1)},
            {TValue::Integer(0), TValue::Integer(0)},
            {TValue::Integer(1), TValue::Integer(1)},
            {TValue::Integer(-1), TValue::Integer(1)},
            {TValue::Integer(Max<i64>()), TValue::Integer(Max<i64>())},
            {TValue::Double(-3.14), TValue::Double(3.14)},
            {TValue::Double(3.14), TValue::Double(3.14)},
        };

        for (const auto& [target, expected] : positiveCases) {
            const auto actual = Abs(ctx, /* globals = */ nullptr, target);
            UNIT_ASSERT_VALUES_EQUAL_C(expected, actual, "target = " << Repr(target));
        }

        UNIT_ASSERT_EXCEPTION(Abs(ctx, /* globals = */ nullptr, TValue::String("-1")), TTypeError);
        UNIT_ASSERT_EXCEPTION(Abs(ctx, /* globals = */ nullptr, TValue::Integer(Min<i64>())), TValueError);
    }

    Y_UNIT_TEST(FirstLast) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        auto iv = [](const i64 x) {
            return TValue::Integer(x);
        };

        const std::tuple<TValue, TValue, TValue> positiveCases[] = {
            {TValue::List({iv(1), iv(2), iv(3)}), iv(1), iv(3)},
            {TValue::List({iv(1)}), iv(1), iv(1)},
            {TValue::List(), TValue::Undefined(), TValue::Undefined()},
            {TValue::Range({0, 5, 1}), iv(0), iv(4)},
            {TValue::Range({0, -5, 1}), TValue::Undefined(), TValue::Undefined()},
            {TValue::Dict({{"foo", TValue::Integer(123)}}), TValue::Undefined(), TValue::Undefined()},
        };

        for (const auto& [target, expectedFirst, expectedLast] : positiveCases) {
            const auto actualFirst = First(ctx, /* globals = */ nullptr, target);
            const auto actualLast = Last(ctx, /* globals = */ nullptr, target);
            UNIT_ASSERT_VALUES_EQUAL_C(expectedFirst, actualFirst, "target = " << Repr(target));
            UNIT_ASSERT_VALUES_EQUAL_C(expectedLast, actualLast, "target = " << Repr(target));
        }
    }

    Y_UNIT_TEST(Join) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        auto strDict = [](const TStringBuf x) {
            return TValue::Dict({{"foo", TValue::String(x)}});
        };

        // we proxy to StrJoin so no need to die for good test coverage
        UNIT_ASSERT_VALUES_EQUAL(Str("a b"), Join(ctx, /* globals = */ nullptr,
                                                  TValue::List({Str("a"), Str("b")}), Str(" "), TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(Str("a b"), Join(ctx, /* globals = */ nullptr,
                                                  TValue::List({strDict("a"), strDict("b")}), Str(" "), Str("foo")));
    }

    Y_UNIT_TEST(List) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, TValue> positiveCases[] = {
            {TValue::List({Str("a")}), TValue::List({Str("a")})},
            {TValue::Range({0, 1, 1}), TValue::List({TValue::Integer(0)})},
            {TValue::Dict({{"foo", TValue::None()}}), TValue::List({Str("foo")})},
        };

        for (const auto& [target, expected] : positiveCases) {
            const auto actual = List(ctx, /* globals = */ nullptr, target);
            UNIT_ASSERT_VALUES_EQUAL_C(expected, actual, "target = " << Repr(target));
        }

        UNIT_ASSERT_EXCEPTION(List(ctx, /* globals = */ nullptr, TValue::Integer(1)), TTypeError);
    }

    Y_UNIT_TEST(LowerUpper) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        // we proxy to StrLower, StrUpper so no need to die for good test coverage
        UNIT_ASSERT_VALUES_EQUAL(Str("превед медвед"), Lower(ctx, /* globals = */ nullptr, Str("Превед медвед")));
        UNIT_ASSERT_VALUES_EQUAL(Str("ПРЕВЕД МЕДВЕД"), Upper(ctx, /* globals = */ nullptr, Str("Превед медвед")));
    }

    Y_UNIT_TEST(Replace) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        // we proxy to StrReplace, so no need to die for good test coverage
        UNIT_ASSERT_VALUES_EQUAL(Str("Прев* м*в*"), Replace(ctx, /* globals = */ nullptr,
                                                      Str("Превед медвед"), Str("ед"), Str("*")));
    }

    Y_UNIT_TEST(Max) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT_VALUES_EQUAL(
            TValue::Integer(4),
            Max(ctx, /* globals = */ nullptr, TValue::List({
                TValue::Integer(3),
                TValue::Integer(4),
            }))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::Double(5.1),
            Max(ctx, /* globals = */ nullptr, TValue::List({
                TValue::Integer(1),
                TValue::Double(5.1),
                TValue::Integer(2),
            }))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            Str("def"),
            Max(ctx, /* globals = */ nullptr, TValue::List({
                Str("aa"),
                Str("abc"),
                Str("def"),
            }))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::Integer(1),
            Max(ctx, /* globals = */ nullptr, TValue::List({
                TValue::Integer(1),
            }))
        );
        UNIT_ASSERT_EXCEPTION(Max(ctx, /* globals = */ nullptr, Str("123")), TTypeError);
        UNIT_ASSERT_EXCEPTION(Max(ctx, /* globals = */ nullptr, TValue::List()), TTypeError);
        UNIT_ASSERT_EXCEPTION(
            Max(ctx, /* globals = */ nullptr, TValue::List({TValue::Integer(1), Str("123")})),
            TTypeError);
    }

    Y_UNIT_TEST(Min) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT_VALUES_EQUAL(
            TValue::Integer(3),
            Min(ctx, /* globals = */ nullptr, TValue::List({
                TValue::Integer(7),
                TValue::Integer(3),
            }))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::Double(4.9),
            Min(ctx, /* globals = */ nullptr, TValue::List({
                TValue::Integer(6),
                TValue::Double(4.9),
                TValue::Integer(8),
            }))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            Str("aa"),
            Min(ctx, /* globals = */ nullptr, TValue::List({
                Str("aa"),
                Str("abc"),
                Str("def"),
            }))
        );
        UNIT_ASSERT_VALUES_EQUAL(
            TValue::Integer(1),
            Min(ctx, /* globals = */ nullptr, TValue::List({
                TValue::Integer(1),
            }))
        );
        UNIT_ASSERT_EXCEPTION(Min(ctx, /* globals = */ nullptr, Str("123")), TTypeError);
        UNIT_ASSERT_EXCEPTION(Min(ctx, /* globals = */ nullptr, TValue::List()), TTypeError);
        UNIT_ASSERT_EXCEPTION(
            Min(ctx, /* globals = */ nullptr, TValue::List({TValue::Integer(1), Str("123")})),
            TTypeError);
    }

    Y_UNIT_TEST(SplitBigNumber) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<ui32, TString> cases[] = {
                        {1, "1"},
                        {21, "21"},
                        {799, "799"},
                        {4781, "4781"}, // don't split numbers with length less than 4
                        {76543, "76 543"},
                        {111112, "111 112"},
                        {9873452, "9 873 452"},
        };

        for (const auto& [target, expected] : cases) {
            UNIT_ASSERT_VALUES_EQUAL_C(Str(expected), SplitBigNumber(ctx, /* globals = */ nullptr, TValue::Integer(target)), "target = " << target);
        }
        UNIT_ASSERT_EXCEPTION(SplitBigNumber(ctx, /* globals = */ nullptr, Str("123")), TTypeError);
        UNIT_ASSERT_EXCEPTION(SplitBigNumber(ctx, /* globals = */ nullptr, TValue::Integer(-12)), TTypeError);
    }

    Y_UNIT_TEST(String) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TValue, TValue> cases[] = {
            {TValue::Undefined(), Str("")},
            {TValue::Bool(false), Str("False")},
            {TValue::Integer(123), Str("123")},
            {TValue::Double(3.14), Str("3.14")},
            {Str("hello"), Str("hello")},
            {TValue::List({Str("hello")}), Str("['hello']")},
            {TValue::Range({0, 3, 1}), Str("xrange(3)")},
            {TValue::Dict({{"foo", Str("bar")}}), Str("{'foo': 'bar'}")},
            {TValue::None(), Str("None")},
        };

        for (const auto& [target, expected] : cases) {
            const auto actual = String(ctx, /* globals = */ nullptr, target);
            UNIT_ASSERT_VALUES_EQUAL_C(expected, actual, "target = " << Repr(target));
        }
    }

    Y_UNIT_TEST(Trim) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        // we proxy to StrStrip so no need to do an extensive test
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("Hello"),
                                 Trim(ctx, /* globals = */ nullptr, TValue::String("  Hello\n ")));
    }

    Y_UNIT_TEST(Length) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(0), Length(ctx, /* globals = */ nullptr, TValue::Undefined()));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(2), Length(ctx, /* globals = */ nullptr,
                                                            TValue::List({TValue::None(), TValue::None()})));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(2), Length(ctx, /* globals = */ nullptr,
                                                            TValue::Dict({{"foo", TValue::None()},
                                                                          {"bar", TValue::None()}})));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(3), Length(ctx, /* globals = */ nullptr, TValue::Range({0, 5, 2})));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(6), Length(ctx, /* globals = */ nullptr, TValue::String("Привет")));

        UNIT_ASSERT_EXCEPTION(Length(ctx, /* globals = */ nullptr, TValue::Integer(123)), TTypeError);
        UNIT_ASSERT_EXCEPTION(Length(ctx, /* globals = */ nullptr, TValue::Double(123)), TTypeError);
        UNIT_ASSERT_EXCEPTION(Length(ctx, /* globals = */ nullptr, TValue::None()), TTypeError);
    }

    Y_UNIT_TEST(NumberOfReadableTokens) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, i64> positiveCases[] = {
            {"", 0},
            {"asdf buio привет", 3},
            {"123 456а", 2},
            {"123 .", 1},
        };

        for (const auto& [target, expected] : positiveCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(expected),
                                       NumberOfReadableTokens(ctx, /* globals = */ nullptr,
                                                              TValue::String(target)),
                                       "target = " << target);
        }

        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(0),
                                 NumberOfReadableTokens(ctx, /* globals = */ nullptr, TValue::Undefined()));

        UNIT_ASSERT_EXCEPTION(NumberOfReadableTokens(ctx, /* globals = */ nullptr, TValue::Integer(123)), TTypeError);
    }

    Y_UNIT_TEST(Map) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        {
            const auto target = TValue::List();
            const auto expected = TValue::List();
            const auto actual = Map(ctx, /* globals = */ nullptr, target, TValue::None(), TValue::String("awol"));
            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }

        {
            const auto target = TValue::List({TValue::Dict({{"foo", TValue::Integer(123)}}),
                                              TValue::Dict({{"foo", TValue::Integer(456)}})});
            const auto expected = TValue::List({TValue::Integer(123), TValue::Integer(456)});
            const auto actual = Map(ctx, /* globals = */ nullptr, target, TValue::None(), TValue::String("foo"));
            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }

        {
            const auto target = TValue::List({TValue::Dict({{"foo", TValue::Integer(123)}}),
                                              TValue::Dict({{"bar", TValue::Integer(456)}})});
            const auto expected = TValue::List({TValue::Integer(123), TValue::Undefined()});
            const auto actual = Map(ctx, /* globals = */ nullptr, target, TValue::None(), TValue::String("foo"));
            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }

        UNIT_ASSERT_EXCEPTION(Map(ctx, /* globals = */ nullptr,
                                  TValue::List(), TValue::String("foo"), TValue::String("foo")),
                              TValueError);
        UNIT_ASSERT_EXCEPTION(Map(ctx, /* globals = */ nullptr,
                                  TValue::List(), TValue::String("foo"), TValue::None()),
                              TValueError);
        UNIT_ASSERT_EXCEPTION(Map(ctx, /* globals = */ nullptr,
                                  TValue::Integer(123), TValue::None(), TValue::String("foo")),
                              TTypeError);
    }

    Y_UNIT_TEST(GetItem) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto pi = TValue::Double(3.14);

        const auto subdict = TValue::Dict({{"bar", TValue::Integer(123)}});
        const auto list = TValue::List({subdict});
        TValue::TDict dict;
        dict["foo"] = subdict;
        dict["baz"] = TValue::None();
        dict["list"] = list;
        const auto target = TValue::Dict(std::move(dict));

        UNIT_ASSERT_VALUES_EQUAL(subdict, GetItem(ctx, /* globals = */ nullptr, target, TValue::String("foo"), pi));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(123),
                                 GetItem(ctx, /* globals = */ nullptr, target, TValue::String("foo.bar"), pi));
        UNIT_ASSERT_VALUES_EQUAL(TValue::None(),
                                 GetItem(ctx, /* globals = */ nullptr, target, TValue::String("baz"), pi));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(123),
                                 GetItem(ctx, /* globals = */ nullptr, target, TValue::String("list.0.bar"), pi));
        UNIT_ASSERT_VALUES_EQUAL(subdict, GetItem(ctx, /* globals = */ nullptr, list, TValue::Integer(0), pi));

        UNIT_ASSERT_VALUES_EQUAL(pi, GetItem(ctx, /* globals = */ nullptr, target, TValue::String("foo.awol"), pi));
        UNIT_ASSERT_VALUES_EQUAL(pi, GetItem(ctx, /* globals = */ nullptr, target, TValue::String("awol"), pi));
        UNIT_ASSERT_VALUES_EQUAL(pi, GetItem(ctx, /* globals = */ nullptr, target, TValue::String("list.1.bar"), pi));

        UNIT_ASSERT_EXCEPTION(GetItem(ctx, /* globals = */ nullptr, target, pi, pi), TTypeError);
    }

    Y_UNIT_TEST(TtsDomain) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        TVector<std::pair<TStringBuf, TText::TFlags>> expectedSpans;
        expectedSpans.push_back({TStringBuf("<[domain music]>"), TText::EFlag::Voice});
        expectedSpans.push_back({TStringBuf("05/07/033"), TText::TFlags{} | TText::EFlag::Voice | TText::EFlag::Text});
        expectedSpans.push_back({TStringBuf("<[/domain]>"), TText::EFlag::Voice});

        const auto actual = TtsDomain(ctx, /* globals = */ nullptr,
                                      TValue::String("05/07/033"), TValue::String("music"));
        UNIT_ASSERT(actual.IsString());
        const auto& actualStr = actual.GetString();
        const TVector<std::pair<TStringBuf, TText::TFlags>> actualSpans(actualStr.begin(), actualStr.end());
        UNIT_ASSERT_VALUES_EQUAL(expectedSpans, actualSpans);

        UNIT_ASSERT_EXCEPTION(TtsDomain(ctx, /* globals = */ nullptr, TValue::String(""), TValue::None()), TTypeError);
    }

    Y_UNIT_TEST(Dates) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ruCtx{env, rng, callStack, "ru"};
        const TCallCtx trCtx{env, rng, callStack, "tr"};
        const auto date = CreateDateSafe(ruCtx, /* globals = */ nullptr,
                                         TValue::Integer(2019), TValue::Integer(8), TValue::Integer(23));

        UNIT_ASSERT(date.IsDict());

        UNIT_ASSERT_VALUES_EQUAL(TValue::String("август"),
                                 HumanMonth(ruCtx, /* globals = */ nullptr, date, TValue::None()));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("августа"),
                                 HumanMonth(ruCtx, /* globals = */ nullptr, date, TValue::String("gent")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("Ağustos"),
                                 HumanMonth(trCtx, /* globals = */ nullptr, date, TValue::None()));

        UNIT_ASSERT_VALUES_EQUAL(TValue::String("пятница"), FormatWeekday(ruCtx, /* globals = */ nullptr, date));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("Cuma"), FormatWeekday(trCtx, /* globals = */ nullptr, date));

        UNIT_ASSERT_VALUES_EQUAL(TValue::None(), CreateDateSafe(ruCtx, /* globals = */ nullptr, TValue::None(), TValue::None(), TValue::None()));
        UNIT_ASSERT_EXCEPTION(HumanMonth(ruCtx, /* globals = */ nullptr, TValue::None(), TValue::None()), TTypeError);
        UNIT_ASSERT_EXCEPTION(HumanMonth(ruCtx, /* globals = */ nullptr, date, TValue::Integer(123)), TTypeError);

        UNIT_ASSERT_EXCEPTION(FormatWeekday(ruCtx, /* globals = */ nullptr, TValue::None()), TTypeError);
    }

    Y_UNIT_TEST(HumanDate) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        int currentYear = NDatetime::ToCivilTime(TInstant::Now(), NDatetime::GetUtcTimeZone()).Year + 1900;

        const auto datePrevYear = CreateDateSafe(ctx, nullptr, TValue::Integer(currentYear - 1), TValue::Integer(8), TValue::Integer(23));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("23 августа " + ToString(currentYear - 1) + " года"), HumanDate(ctx, /* globals = */ nullptr, datePrevYear));

        const auto dateCurrYear = CreateDateSafe(ctx, nullptr, TValue::Integer(currentYear), TValue::Integer(8), TValue::Integer(23));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("23 августа"), HumanDate(ctx, /* globals = */ nullptr, dateCurrYear));

        const auto dateNextYear = CreateDateSafe(ctx, nullptr, TValue::Integer(currentYear + 1), TValue::Integer(8), TValue::Integer(23));
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("23 августа " + ToString(currentYear + 1) + " года"), HumanDate(ctx, /* globals = */ nullptr, dateNextYear));
    }

    Y_UNIT_TEST(HumanDayRel) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        auto checkDay = [&ctx](NDatetime::TSimpleTM::EField diffType, i32 diffCount, TMaybe<TValue> expected) {
            auto time = NDatetime::ToCivilTime(TInstant::Now(), NDatetime::GetUtcTimeZone());
            if (diffCount) {
                time.Add(diffType, diffCount);
            }

            const auto date = CreateDateSafe(ctx, nullptr, TValue::Integer(time.Year + 1900), TValue::Integer(time.Mon + 1), TValue::Integer(time.MDay));
            if (expected) {
                UNIT_ASSERT_VALUES_EQUAL(*expected, HumanDayRel(ctx, /* globals = */ nullptr, date));
            } else {
                UNIT_ASSERT_VALUES_EQUAL(HumanDate(ctx, /* globals = */ nullptr, date), HumanDayRel(ctx, /* globals = */ nullptr, date));
            }
        };

        checkDay(NDatetime::TSimpleTM::F_DAY, 0, TValue::String("сегодня"));
        checkDay(NDatetime::TSimpleTM::F_DAY, 1, TValue::String("завтра"));
        checkDay(NDatetime::TSimpleTM::F_DAY, 2, TValue::String("послезавтра"));
        checkDay(NDatetime::TSimpleTM::F_DAY, 3, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_DAY, -1, TValue::String("вчера"));
        checkDay(NDatetime::TSimpleTM::F_DAY, -2, TValue::String("позавчера"));
        checkDay(NDatetime::TSimpleTM::F_DAY, -3, Nothing());  // no special datetime

        checkDay(NDatetime::TSimpleTM::F_MON, -1, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_MON, 1, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_YEAR, -1, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_YEAR, 1, Nothing());  // no special datetime
    }

    Y_UNIT_TEST(HumanDayRel2) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto date = CreateDateSafe(ctx, nullptr, TValue::Integer(1960), TValue::Integer(10), TValue::Integer(5));
        TValue tzName = TValue::String("UTC");

        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "year").GetInteger(), 1960);
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "month").GetInteger(), 10);
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "day").GetInteger(), 5);

        UNIT_ASSERT_VALUES_EQUAL(HumanDayRel(ctx, /* globals = */ nullptr, date, tzName).GetString().GetStr(), "5 октября 1960 года");

        const auto date1 = CreateDateSafe(ctx, nullptr, TValue::Integer(1972), TValue::Integer(8), TValue::Integer(1));
        UNIT_ASSERT_VALUES_EQUAL(HumanDayRel(ctx, /* globals = */ nullptr, date1, tzName).GetString().GetStr(), "1 августа 1972 года");

        const auto date2 = CreateDateSafe(ctx, nullptr, TValue::Integer(1972), TValue::Integer(5), TValue::Integer(1));
        UNIT_ASSERT_VALUES_EQUAL(HumanDayRel(ctx, /* globals = */ nullptr, date2, tzName).GetString().GetStr(), "1 мая 1972 года");

        const auto date3 = CreateDateSafe(ctx, nullptr, TValue::Integer(1970), TValue::Integer(1), TValue::Integer(1));
        UNIT_ASSERT_VALUES_EQUAL(HumanDayRel(ctx, /* globals = */ nullptr, date3, tzName).GetString().GetStr(), "1 января 1970 года");

        const auto date4 = CreateDateSafe(ctx, nullptr, TValue::Integer(1969), TValue::Integer(12), TValue::Integer(31));
        UNIT_ASSERT_VALUES_EQUAL(HumanDayRel(ctx, /* globals = */ nullptr, date4, tzName).GetString().GetStr(), "31 декабря 1969 года");
    }

    Y_UNIT_TEST(HumanDayRel3) {
        const TVector<std::pair<TString, TString>> testStrings = {
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":1}", "через год"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":2}", "через 2 года"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":3}", "через 3 года"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":4}", "через 4 года"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":5}", "через 5 лет"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":6}", "через 6 лет"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":7}", "через 7 лет"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":8}", "через 8 лет"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":9}", "через 9 лет"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":10}", "через 10 лет"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":11}", "через 11 лет"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-1}", "год назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-2}", "2 года назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-3}", "3 года назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-4}", "4 года назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-5}", "5 лет назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-6}", "6 лет назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-7}", "7 лет назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-8}", "8 лет назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-9}", "9 лет назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-10}", "10 лет назад"),
            std::pair<TString, TString>("{\"years_relative\":true, \"years\":-11}", "11 лет назад"),
        };

        for (const auto& it: testStrings) {
            const TEnvironment env;
            TFakeRng rng;
            TCallStack callStack;
                        const TCallCtx ctx{env, rng, callStack, RuLanguage};

            TMaybe<TSysDatetimeParser> sourceDate = TSysDatetimeParser::Parse(it.first);
            TValue date = TValue::FromJsonValue(sourceDate->GetAsJsonDatetime());
            TValue result = RenderDatetimeRaw(ctx, /* globals = */ nullptr, date);
            UNIT_ASSERT_VALUES_EQUAL(it.second, GetAttrLoad(result, "text").GetString().GetStr());
        }
    }

    Y_UNIT_TEST(HumanDayRelWithMockedTime) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto oldInstant = TInstant::ParseIso8601("2005-08-09T18:31:42"); // year 2005, August 09; 18:31:42
        const auto mockedTime = TValue(static_cast<i64>(oldInstant.MicroSeconds())); // cast ui64 to i64

        auto checkDay = [&ctx, oldInstant, mockedTime](NDatetime::TSimpleTM::EField diffType, i32 diffCount, TMaybe<TValue> expected) {
            auto time = NDatetime::ToCivilTime(oldInstant, NDatetime::GetUtcTimeZone());
            if (diffCount) {
                time.Add(diffType, diffCount);
            }

            const auto date = CreateDateSafe(ctx, nullptr, TValue::Integer(time.Year + 1900), TValue::Integer(time.Mon + 1), TValue::Integer(time.MDay));
            if (expected) {
                UNIT_ASSERT_VALUES_EQUAL(*expected, HumanDayRel(ctx, /* globals = */ nullptr, date, /* timezone = */ TValue::None(), /* mockedTime = */ mockedTime));
            } else {
                UNIT_ASSERT_VALUES_EQUAL(HumanDate(ctx, /* globals = */ nullptr, date),
                                         HumanDayRel(ctx, /* globals = */ nullptr, date, /* timezone = */ TValue::None(), /* mockedTime = */ mockedTime));
            }
        };

        checkDay(NDatetime::TSimpleTM::F_DAY, 0, TValue::String("сегодня"));
        checkDay(NDatetime::TSimpleTM::F_DAY, 1, TValue::String("завтра"));
        checkDay(NDatetime::TSimpleTM::F_DAY, 2, TValue::String("послезавтра"));
        checkDay(NDatetime::TSimpleTM::F_DAY, 3, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_DAY, -1, TValue::String("вчера"));
        checkDay(NDatetime::TSimpleTM::F_DAY, -2, TValue::String("позавчера"));
        checkDay(NDatetime::TSimpleTM::F_DAY, -3, Nothing());  // no special datetime

        checkDay(NDatetime::TSimpleTM::F_MON, -1, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_MON, 1, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_YEAR, -1, Nothing());  // no special datetime
        checkDay(NDatetime::TSimpleTM::F_YEAR, 1, Nothing());  // no special datetime
    }

    Y_UNIT_TEST(DateTimezoneSwitch) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        // 13 July 22:00 UTC (default timezone)
        auto utcDate = CreateDateSafe(ctx, nullptr, TValue::Integer(2019), TValue::Integer(7), TValue::Integer(13));
        utcDate.GetMutableDict()["hour"] = TValue::Integer(22);
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("13 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, utcDate));

        // 13 July 23:00 London
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("13 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, utcDate, TValue::String("Europe/London")));

        // 14 July 00:00 Berlin
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("14 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, utcDate, TValue::String("Europe/Berlin")));

        // 14 July 01:00 Moscow
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("14 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, utcDate, TValue::String("Europe/Moscow")));


        // 14 July 01:00 Moscow
        auto moscowDate = CreateDateSafe(ctx, nullptr, TValue::Integer(2019), TValue::Integer(7), TValue::Integer(14));
        moscowDate.GetMutableDict()["hour"] = TValue::Integer(1);
        moscowDate.GetMutableDict()["tzinfo"] = TValue::String("Europe/Moscow");
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("14 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, moscowDate, TValue::String("Europe/Moscow")));

        // 14 July 00:00 Berlin
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("14 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, moscowDate, TValue::String("Europe/Berlin")));

        // 13 July 23:00 London
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("13 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, moscowDate, TValue::String("Europe/London")));

        // 13 July 22:00 UTC
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("13 июля 2019 года"), HumanDate(ctx, /* globals = */ nullptr, moscowDate, TValue::String("UTC")));
    }

    Y_UNIT_TEST(TimestampToDatetime) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<int, int, int, int, int, int, int, TStringBuf, TStringBuf, TStringBuf> traitsCases[] = {
            // timestamp,         year, month, day, hour, minute, second, weekday, humanMonth, timezone
            {0, 1970, 1, 1, 0, 0, 0, "четверг", "январь", "UTC"},
            {-1, 1969, 12, 31, 23, 59, 59, "среда", "декабрь", "UTC"},
            {-3665, 1969, 12, 31, 22, 58, 55, "среда", "декабрь", "UTC"},
            {3665, 1970, 1, 1, 1, 1, 5, "четверг", "январь", "UTC"},
            {1000000, 1970, 1, 12, 13, 46, 40, "понедельник", "январь", "UTC"},
            {10000000, 1970, 4, 26, 17, 46, 40, "воскресенье", "апрель", "UTC"},
            {50000000, 1971, 8, 2, 16, 53, 20, "понедельник", "август", "UTC"},
            {100000000, 1973, 3, 3, 9, 46, 40, "суббота", "март", "UTC"},
            {500000000, 1985, 11, 5, 0, 53, 20, "вторник", "ноябрь", "UTC"},
            {1000000000, 2001, 9, 9, 1, 46, 40, "воскресенье", "сентябрь", "UTC"},
            {1000000000, 2001, 9, 9, 5, 46, 40, "воскресенье", "сентябрь", "Europe/Moscow"},
            {1500000000, 2017, 7, 14, 2, 40, 0, "пятница", "июль", "UTC"},
            {1500000000, 2017, 7, 14, 5, 40, 0, "пятница", "июль", "Europe/Moscow"},
        };
        for (const auto& [timestamp, year, month, day, hour, minute, second, weekday, humanMonth, timezone] :
             traitsCases) {
            const auto date = TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::Integer(timestamp),
                                                  TValue::String(timezone));
            UNIT_ASSERT_C(date.IsDict(), "timestamp = " << timestamp);
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(year),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("year"), TValue::None()),
                                       "timestamp = " << timestamp << ", year = " << year);
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(month),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("month"), TValue::None()),
                                       "timestamp = " << timestamp << ", month = " << month);
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(day),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("day"), TValue::None()),
                                       "timestamp = " << timestamp << ", day = " << day);
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(hour),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("hour"), TValue::None()),
                                       "timestamp = " << timestamp << ", hour = " << hour);
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(minute),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("minute"), TValue::None()),
                                       "timestamp = " << timestamp << ", minute = " << minute);
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(second),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("second"), TValue::None()),
                                       "timestamp = " << timestamp << ", second = " << second);

            UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(timezone),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("tzinfo"), TValue::None()),
                                       "timestamp = " << timestamp << " tzinfo = " << timezone);

            UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(weekday),
                                       FormatWeekday(ctx, /* globals = */ nullptr, date),
                                       "timestamp = " << timestamp << ", weekday = " << weekday);

            UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(humanMonth),
                                       HumanMonth(ctx, /* globals = */ nullptr,
                                                  date, TValue::None()),
                                       "timestamp = " << timestamp << ", month = " << humanMonth);
        }

        const auto date_default = TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::Integer(1000000));

        UNIT_ASSERT_VALUES_EQUAL_C(TValue::String("UTC"),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date_default, TValue::String("tzinfo"), TValue::None()),
                                       "timestamp = " << 1000000 << " wrong default timezone");

        UNIT_ASSERT_EXCEPTION(TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::None()), TTypeError);

        UNIT_ASSERT_EXCEPTION(TimestampToDatetime(ctx, /* globals = */ nullptr,
                                                  TValue::String("тысяча")), TTypeError);

        UNIT_ASSERT_EXCEPTION(TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::Integer(1000000000),
                              TValue::String("abrcadabra")), NDatetime::TInvalidTimezone);

        UNIT_ASSERT_EXCEPTION(TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::Integer(1000000000),
                              TValue::Integer(1)), TTypeError);
    }

    Y_UNIT_TEST(Strftime) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        // UTC 2001-09-09 01:46:40
        const auto date2001 = TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::Integer(1000000000));

        // 30.07.1419
        const auto  bareDate1419 = CreateDateSafe(ctx, /* globals = */ nullptr,
                                                  TValue::Integer(1419), TValue::Integer(7), TValue::Integer(30));
        // Moscow 2017-07-14 05:40:00 (02:40:00 UTC)
        const auto msk2017 = TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::Integer(1500000000),
                                                 TValue::String("Europe/Moscow"));

        UNIT_ASSERT_VALUES_EQUAL(DatetimeStrftime(ctx, /* globals = */ nullptr, date2001,
                                                  TValue::String("%Y-%m-%d %H:%M:%S")),
                                 TValue::String("2001-09-09 01:46:40"));

        UNIT_ASSERT_VALUES_EQUAL(DatetimeStrftime(ctx, /* globals = */ nullptr, date2001,
                                                  TValue::String("%Y-%m-%d %H-%M-%S")),
                                 TValue::String("2001-09-09 01-46-40"));

        UNIT_ASSERT_VALUES_EQUAL(DatetimeStrftime(ctx, /* globals = */ nullptr, date2001, TValue::String("%Y %m %d")),
                                 TValue::String("2001 09 09"));

        UNIT_ASSERT_VALUES_EQUAL(DatetimeStrftime(ctx, /* globals = */ nullptr, date2001, TValue::String("%z")),
                                 TValue::String("+0000"));

        UNIT_ASSERT_EXCEPTION(DatetimeStrftime(ctx, /* globals = */ nullptr, date2001, TValue::None()), TTypeError);


        UNIT_ASSERT_VALUES_EQUAL(DatetimeStrftime(ctx, /* globals = */ nullptr, bareDate1419,
                                                  TValue::String("%Y-%m-%d %H:%M:%S")),
                                 TValue::String("1419-07-30 00:00:00"));

        UNIT_ASSERT_EXCEPTION(DatetimeStrftime(ctx, /* globals = */ nullptr, TValue::Integer(0),
                                       TValue::String("%Y-%m-%d %H:%M:%S")), TTypeError);

        UNIT_ASSERT_VALUES_EQUAL(DatetimeStrftime(ctx, /* globals = */ nullptr, msk2017, TValue::String("%Y-%m-%d %H:%M:%S %z")),
                                 TValue::String("2017-07-14 05:40:00 +0300"));
    }

    Y_UNIT_TEST(Datetime) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<i64, i64, i64, i64, i64, i64, i64, TStringBuf> cases[] = {
            // year, month, day, hour, minute, second, microsecond, strDate
            {1981, 4, 5, 20, 13, 15, 100000, "1981-04-05 20:13:15 UTC+0000"},
            {1981, 4, 0, 20, 13, 15, 100000, "1981-03-31 20:13:15 UTC+0000"},
            {1981, 4, 5, -2, 13, 15, 100000,  "1981-04-04 22:13:15 UTC+0000"},
            {1960, 10, 5, 12, 10, 46, 100000,  "1960-10-05 12:10:46 UTC+0000"},
            {2020, 2, 7, 10, 30, 17, 0, "2020-02-07 10:30:17 UTC+0000"},
            {2020, 2, 30, 10, 30, 17, 0, "2020-03-01 10:30:17 UTC+0000"},
            {9999, 2, 7, 10, 30, 17, 0, "9999-02-07 10:30:17 UTC+0000"},
            {9, 4, 5, 20, 13, 15, 100000, "9-04-05 20:13:15 UTC+0000"},
            {-27, 4, 5, 20, 13, 15, 100000, "-27-04-05 20:13:15 UTC+0000"},
        };
        for (const auto& [year, month, day, hour, minute, second, microsecond, strDate] : cases) {
            const auto date = Datetime(ctx, /* globals = */ nullptr,
                                       TValue::Integer(year), TValue::Integer(month), TValue::Integer(day),
                                       TValue::Integer(hour), TValue::Integer(minute), TValue::Integer(second),
                                       TValue::Integer(microsecond));
            UNIT_ASSERT_C(date.IsDict(), "datetime - " << strDate);

            UNIT_ASSERT_VALUES_EQUAL_C(DatetimeStrftime(ctx, /* globals = */ nullptr, date,
                                                        TValue::String("%Y-%m-%d %H:%M:%S %Z%z")),
                                     TValue::String(strDate), "datetime = " << strDate);
        }

        const std::tuple<TStringBuf, TValue, TValue, TValue, TValue, TValue, TValue, TValue> typeErrorCases[] = {
            // invalidField, year, month, day, hour, minute, second, microsecond
            {"year", TValue::None(), TValue::Integer(4), TValue::Integer(5),
             TValue::Integer(20), TValue::Integer(13), TValue::Integer(15), TValue::Integer(10000)},
            {"month", TValue::Integer(1981), TValue::None(), TValue::Integer(5),
             TValue::Integer(20), TValue::Integer(13), TValue::Integer(15), TValue::Integer(10000)},
            {"day", TValue::Integer(1981), TValue::Integer(4), TValue::None(),
             TValue::Integer(20), TValue::Integer(13), TValue::Integer(15), TValue::Integer(10000)},
            {"hour", TValue::Integer(1981), TValue::Integer(4), TValue::Integer(5),
             TValue::None(), TValue::Integer(13), TValue::Integer(15), TValue::Integer(10000)},
            {"minute", TValue::Integer(1981), TValue::Integer(4), TValue::Integer(5),
             TValue::Integer(20), TValue::None(), TValue::Integer(15), TValue::Integer(10000)},
            {"second", TValue::Integer(1981), TValue::Integer(4), TValue::Integer(5),
             TValue::Integer(20), TValue::Integer(13), TValue::None(), TValue::Integer(10000)},
            {"microsecond", TValue::Integer(1981), TValue::Integer(4), TValue::Integer(5),
             TValue::Integer(20), TValue::Integer(13), TValue::Integer(15), TValue::None()},
        };
        for (const auto& [invalidField, year, month, day, hour, minute, second, microsecond] : typeErrorCases) {
            UNIT_ASSERT_EXCEPTION_C(Datetime(ctx, /* globals = */ nullptr, year, month, day, hour, minute, second,
                                           microsecond), TTypeError, "invalid field: " << invalidField);
        };

        UNIT_ASSERT_EXCEPTION(Datetime(ctx, /* globals = */ nullptr,
                              TValue::Integer(1981), TValue::Integer(4), TValue::Integer(5),
                              TValue::Integer(20), TValue::Integer(13), TValue::Integer(15), TValue::Integer(1000000)),
                              TValueError);

        UNIT_ASSERT_EXCEPTION(Datetime(ctx, /* globals = */ nullptr,
                              TValue::Integer(1981), TValue::Integer(4), TValue::Integer(5),
                              TValue::Integer(20), TValue::Integer(13), TValue::Integer(15), TValue::Integer(-100000)),
                              TValueError);
    }

    Y_UNIT_TEST(ParseTimezone) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT_EXCEPTION(ParseTz(ctx, /* globals = */ nullptr, TValue::None()), TTypeError);

        const auto utc = ParseTz(ctx, /* globals = */ nullptr, TValue::String("UTC"));
        UNIT_ASSERT(utc.IsString());

        const std::tuple<TStringBuf, TStringBuf> cases[] = {
            // pattern, stored
            {"UTC", "UTC"},
            {"Australia/Sydney", "Australia/Sydney"},
            {"America/New_York", "America/New_York"},
            {"Europe/Moscow", "Europe/Moscow"},
            {"Europe/London", "Europe/London"},
            {"Asia/Tokyo", "Asia/Tokyo"},
            {"America/Los_Angeles", "America/Los_Angeles"},
        };
        for (const auto& [pattern, stored] : cases) {
            UNIT_ASSERT_VALUES_EQUAL(ParseTz(ctx, /* globals = */ nullptr, TValue::String(pattern)), TValue::String(stored));
        }

        UNIT_ASSERT_EXCEPTION(ParseTz(ctx, /* globals = */ nullptr, TValue::String("hell")), NDatetime::TInvalidTimezone);
        UNIT_ASSERT_EXCEPTION(ParseTz(ctx, /* globals = */ nullptr, TValue::String("europe/moscow")),
                              NDatetime::TInvalidTimezone);
    }

    Y_UNIT_TEST(Localize) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        // UTC 2001-09-09 01:46:40
        const auto utc2001 = TimestampToDatetime(ctx, /* globals = */ nullptr, TValue::Integer(1000000000));

        // datetime without tzinfo
        const auto date2020 = Datetime(ctx, /* globals = */ nullptr,
                                       TValue::Integer(2020), TValue::Integer(2), TValue::Integer(7),
                                       TValue::Integer(10), TValue::Integer(30), TValue::Integer(17));

        const auto bareDate2000 = CreateDateSafe(ctx, /* globals = */ nullptr,
                                                 TValue::Integer(2000), TValue::Integer(8), TValue::Integer(23));

        const std::tuple<TValue, TStringBuf, TStringBuf, TStringBuf> cases[] = {
            // date, timezone_name, date_pattern, dateStr
            {date2020, "UTC", "%Y-%m-%d %H:%M:%S %z", "2020-02-07 10:30:17 +0000"},
            {date2020, "Australia/Sydney", "%Y-%m-%d %H:%M:%S %z", "2020-02-07 10:30:17 +1100"},
            {date2020, "America/New_York", "%Y-%m-%d %H:%M:%S %z", "2020-02-07 10:30:17 -0500"},
            {date2020, "Europe/Moscow", "%Y-%m-%d %H:%M:%S %z", "2020-02-07 10:30:17 +0300"},
            {date2020, "Europe/Moscow", "%z", "+0300"},
            {date2020, "Europe/Moscow", "%Z%z", "MSK+0300"},
            {date2020, "Europe/London", "%Y-%m-%d %H:%M:%S %z", "2020-02-07 10:30:17 +0000"},
            {date2020, "Asia/Tokyo", "%Y-%m-%d %H:%M:%S %z", "2020-02-07 10:30:17 +0900"},
            {date2020, "America/Los_Angeles", "%Y-%m-%d %H:%M:%S %z", "2020-02-07 10:30:17 -0800"},
            {date2020, "Europe/Berlin", "%z", "+0100"},
            {bareDate2000, "Europe/Moscow", "%Z%z", "MSD+0400"},
            {bareDate2000, "Europe/Berlin", "%z", "+0200"},
        };
        for (const auto& [date, name, pattern, dateStr] : cases) {
            const auto tz = ParseTz(ctx, /* globals = */ nullptr, TValue::String(name));
            const auto localizedDate = Localize(ctx, /* globals = */ nullptr, tz, date);
            UNIT_ASSERT(date.IsDict());
            UNIT_ASSERT_VALUES_EQUAL_C(DatetimeStrftime(ctx, /* globals = */ nullptr, localizedDate,
                                                TValue::String(pattern)), TValue::String(dateStr),
                                       "datetime " << dateStr);
            UNIT_ASSERT_EXCEPTION_C(Localize(ctx, /* globals = */ nullptr, tz, localizedDate), TValueError,
                                    "datetime " << dateStr);
        }

        const auto msk = ParseTz(ctx, /* globals = */ nullptr, TValue::String("Europe/Moscow"));
        const auto utc = ParseTz(ctx, /* globals = */ nullptr, TValue::String("UTC"));
        const auto cet = ParseTz(ctx, /* globals = */ nullptr, TValue::String("Europe/Amsterdam"));

        const std::tuple<TValue, TValue> typeErrorCases[] = {
            {TValue::None(), date2020},
            {utc, TValue::None()},
            {TValue::None(), TValue::None()},
        };
        for (const auto& [tz, date] : typeErrorCases) {
            UNIT_ASSERT_EXCEPTION(Localize(ctx, /* globals = */ nullptr, tz, date), TTypeError);
        }

        const std::tuple<TValue, TValue, TValue> valueErrorCases[] = {
            {utc, cet, date2020},
            {utc, msk, date2020},
            {msk, utc, date2020},
            {msk, cet, date2020},
            {cet, msk, date2020},
            {cet, msk, bareDate2000},
            {utc, cet, bareDate2000},
        };
        for (const auto& [tzOne, tzTwo, date] : valueErrorCases) {
            const auto localizedDate = Localize(ctx, /* globals = */ nullptr, tzOne, date);
            UNIT_ASSERT_EXCEPTION(Localize(ctx, /* globals = */ nullptr, tzTwo, localizedDate), TValueError);
        }

        UNIT_ASSERT_EXCEPTION(Localize(ctx, /* globals = */ nullptr, msk, utc2001), TValueError);
    }

    Y_UNIT_TEST(Strptime) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, TStringBuf, TStringBuf> cases[] = {
            // dateString,             dateFormat,                  trueDate
            {"2001-09-09 01:46:40", "%Y-%m-%d %H:%M:%S", "2001-09-09 01:46:40"},
            {"2001 09 09 01:46:40", "%Y %m %d %H:%M:%S", "2001-09-09 01:46:40"},
            {"2001-09-09 01-46-40", "%Y-%m-%d %H-%M-%S", "2001-09-09 01:46:40"},
            {"2001-09-09 01 46 40", "%Y-%m-%d %H %M %S", "2001-09-09 01:46:40"},
            {"2001-09-09", "%Y-%m-%d", "2001-09-09 00:00:00"},
            {"1912-04-14", "%Y-%m-%d", "1912-04-14 00:00:00"},
            {"1912-04-14 23:40", "%Y-%m-%d %H:%M", "1912-04-14 23:40:00"},
            {"2001-09-09 01:46 UTC", "%Y-%m-%d %H:%M %Z", "2001-09-09 01:46:00"},
            {"2001-09-09 01:46 +0000", "%Y-%m-%d %H:%M %z", "2001-09-09 01:46:00"},
        };
        for (const auto& [dateString, dateFormat, trueDate] : cases) {
            const auto date = DatetimeStrptime(ctx, /* globals = */ nullptr, TValue::String(dateString),
                                               TValue::String(dateFormat));
            UNIT_ASSERT(date.IsDict());

            const auto observedDate = DatetimeStrftime(ctx, /* globals = */ nullptr, date, TValue::String("%Y-%m-%d %H:%M:%S"));
            UNIT_ASSERT_VALUES_EQUAL_C(observedDate.GetString().GetStr(), trueDate,
                                       "date " << trueDate << ", format " << dateFormat);

            UNIT_ASSERT_VALUES_EQUAL_C(TValue::None(),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("tzinfo"), TValue::None()),
                                       "date " << trueDate << ", format " << dateFormat);

            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(0),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("microsecond"), TValue::None()),
                                       "date " << trueDate << ", format " << dateFormat);
        }

        const std::tuple<TStringBuf, TStringBuf> valueErrorCases[] = {
            // dateString,             dateFormat
            {"2001 09 09 01:46:40", "%Y-%m-%d %H:%M:%S"},
            {"2001 09 09 01-46-40", "%Y %m %d %H:%M:%S"},
            {"2001-09-09 01:46:40", "%Y-%m-%d %H-%M-%S"},
            {"2001 09 09 01 46 40", "%Y-%m-%d %H %M %S"},
            {"2001-09-09 01:46", "%Y-%m-%d %H:%M:%S"},
            {"2001-09", "%Y-%m-%d"},
            {"2001-09-09 01:46:40", "%Y-%m-%d %H:%M"},
        };
        for (const auto& [dateString, dateFormat] : valueErrorCases) {
            UNIT_ASSERT_EXCEPTION_C(DatetimeStrptime(ctx, /* globals = */ nullptr,
                                                     TValue::String(dateString), TValue::String(dateFormat)),
                                    TValueError, "date string " << dateString);
        }

        UNIT_ASSERT_EXCEPTION(DatetimeStrptime(ctx, /* globals = */ nullptr, TValue::Integer(0),
                                               TValue::String("%Y-%m-%d %H:%M:%S")), TTypeError);
        UNIT_ASSERT_EXCEPTION(DatetimeStrptime(ctx, /* globals = */ nullptr, TValue::String("2000-11-01"), TValue::Integer(0)),
                              TTypeError);
    }

    Y_UNIT_TEST(ParseDt) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, TStringBuf> cases[] = {
            // dateString,                     trueDate
            {"2001-09-09 01:46:40", "2001-09-09 01:46:40"},
            {"2001-09-09 01:46", "2001-09-09 01:46:00"},
            {"2001-09-09 01", "2001-09-09 01:00:00"},
            {"2001 09 09 01:46:40", "2001-09-09 01:46:40"},
            {"2001-09-09", "2001-09-09 00:00:00"},
            {"2001/09/09", "2001-09-09 00:00:00"},
            {"2001.09.09", "2001-09-09 00:00:00"},
            {"1970.01.01", "1970-01-01 00:00:00"},
            {"1969.01.01", "1969-01-01 00:00:00"},
            {"1912.04.14", "1912-04-14 00:00:00"},
            {"1912.04.14 23:40", "1912-04-14 23:40:00"},
        };
        for (const auto& [dateString, trueDate] : cases) {
            const auto date = ParseDt(ctx, /* globals = */ nullptr, TValue::String(dateString));
            UNIT_ASSERT(date.IsDict());

            const auto observedDate = DatetimeStrftime(ctx, /* globals = */ nullptr, date, TValue::String("%Y-%m-%d %H:%M:%S"));
            UNIT_ASSERT_VALUES_EQUAL_C(observedDate.GetString().GetStr(), trueDate,
                                       "true date " << trueDate << ", input string " << dateString);

            UNIT_ASSERT_VALUES_EQUAL_C(TValue::None(),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("tzinfo"), TValue::None()),
                                       "true date " << trueDate << ", input string " << dateString);

            UNIT_ASSERT_VALUES_EQUAL_C(TValue::Integer(0),
                                       DictGet(ctx, /* globals = */ nullptr,
                                               date, TValue::String("microsecond"), TValue::None()),
                                       "true date " << trueDate << ", input string " << dateString);
        }

        const std::vector<TStringBuf> valueErrorCases = {
            // dateString,
            "2001 09 09 01-46-40",
            "2001 09 09 01 46 40",
            "2001-09-09 01:46-00",
            "2001-09",
            "2001-09-09 01-46-40",
            "2001-09-09 01 46 40",
            "2001:09:09 01:46:40",
            "2001:09:09 01:46:40 CET",
            "2001:09:09 01:46:40 UTC",
            "2001:09:09 01:46:40 +0000",
            "2001:09:09 01:46:40 +0100",
            "9 Sep 2001",
            "14 Feb 2020 17:34",
            "11 Oct 2018 08:29:09 +0000",
            "Sun Nov  6 08:49:37 1994",
        };
        for (const auto& dateString : valueErrorCases) {
            UNIT_ASSERT_EXCEPTION_C(ParseDt(ctx, /* globals = */ nullptr, TValue::String(dateString)),
                                    TValueError, "date string " << dateString);
        }

        UNIT_ASSERT_EXCEPTION(ParseDt(ctx, /* globals = */ nullptr, TValue::Integer(0)), TTypeError);
    }

    Y_UNIT_TEST(Isoweekday) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const std::tuple<TStringBuf, TValue> cases[] = {
            // dateString,            weekday
            {"1912-04-15", TValue::Integer(1)},
            {"2011-01-11", TValue::Integer(2)},
            {"1961-04-12", TValue::Integer(3)},
            {"1970-01-01", TValue::Integer(4)},
            {"2018-07-13", TValue::Integer(5)},
            {"1242-04-05", TValue::Integer(6)},
            {"1941-12-07", TValue::Integer(7)},
        };
        for (const auto& [dateString, weekday] : cases) {
            const auto date = DatetimeStrptime(ctx, /* globals = */ nullptr,
                                                     TValue::String(dateString), TValue::String("%Y-%m-%d"));
            UNIT_ASSERT_VALUES_EQUAL_C(DatetimeIsoweekday(ctx, /* globals = */ nullptr, date), weekday,
                                       "date " << dateString << " weekday " << weekday.GetInteger());
        }

        UNIT_ASSERT_EXCEPTION(DatetimeIsoweekday(ctx, /* globals = */ nullptr, TValue::Integer(1)), TTypeError);
    }

    Y_UNIT_TEST(Pluralize) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT_VALUES_EQUAL(TValue::String("маршрута"),
                                 Pluralize(ctx, /* globals = */ nullptr,
                                           TValue::String("маршрут"), TValue::Integer(2), TValue::String("nomn")));
        UNIT_ASSERT_EXCEPTION(Pluralize(ctx, /* globals = */ nullptr,
                                        TValue::None(), TValue::Integer(2), TValue::String("nomn")),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(Pluralize(ctx, /* globals = */ nullptr,
                                        TValue::String(""), TValue::None(), TValue::String("nomn")),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(Pluralize(ctx, /* globals = */ nullptr,
                                        TValue::String(""), TValue::Integer(-2), TValue::String("nomn")),
                              TValueError);
        UNIT_ASSERT_EXCEPTION(Pluralize(ctx, /* globals = */ nullptr,
                                        TValue::String(""), TValue::Double(-2), TValue::String("nomn")),
                              TValueError);
        UNIT_ASSERT_EXCEPTION(Pluralize(ctx, /* globals = */ nullptr,
                                        TValue::String(""), TValue::Integer(2), TValue::None()),
                              TTypeError);
    }

    Y_UNIT_TEST(Singularize) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        UNIT_ASSERT_VALUES_EQUAL(TValue::String("маршрут"),
                                 Singularize(ctx, /* globals = */ nullptr,
                                             TValue::String("маршрута"), TValue::Integer(2)));
        UNIT_ASSERT_EXCEPTION(Singularize(ctx, /* globals = */ nullptr, TValue::None(), TValue::Integer(2)),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(Singularize(ctx, /* globals = */ nullptr, TValue::String(""), TValue::None()),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(Singularize(ctx, /* globals = */ nullptr, TValue::String(""), TValue::Integer(-2)),
                              TValueError);
    }

    Y_UNIT_TEST(CityPrepcase) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const TValue geoWithPreps = TValue::Dict({
            {"city_cases", TValue::Dict({
                {"preposition", TValue::String("в")},
                {"prepositional", TValue::String("Санкт-Петербурге")}
            })},
            {"city", TValue::String("Санкт-Петербург")},
        });
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("в Санкт-Петербурге"), CityPrepcase(ctx, /* globals = */ nullptr, geoWithPreps));

        const TValue geoWithoutPreps = TValue::Dict({
            {"city", TValue::String("Рязань")},
        });
        UNIT_ASSERT_VALUES_EQUAL(TValue::String("в городе Рязань"), CityPrepcase(ctx, /* globals = */ nullptr, geoWithoutPreps));

        UNIT_ASSERT_EXCEPTION(CityPrepcase(ctx, /* globals = */ nullptr, TValue::String("ololo")), TTypeError);

        const TValue geoWithBrokenPreps = TValue::Dict({
            {"city_cases", TValue::String("something")},
            {"city", TValue::String("Санкт-Петербург")},
        });
        UNIT_ASSERT_EXCEPTION(CityPrepcase(ctx, /* globals = */ nullptr, geoWithBrokenPreps), TTypeError);

        const TValue geoWithMissingPreps = TValue::Dict();
        UNIT_ASSERT_EXCEPTION(CityPrepcase(ctx, /* globals = */ nullptr, geoWithMissingPreps), TValueError);
    }

    Y_UNIT_TEST(MusicTitleShorten) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        std::tuple<TStringBuf, TStringBuf> cases[] = {
            {"", ""},
            {"(", "("},
            {")", ")"},
            {"),", ")"},
            {"(,", "("},
            {"()", "()"},
            {"(,)", "(,)"},
            {"(;)", "(;)"},
            {"(;),", ","},
            {"   ((((  ", "(((("},
            {"  ) ( ", ") ("},
            {"  (()((", "(("},
            {"just text", "just text"},
            {" (just text) ", "(just text)"},
            {" , just text", ", just text"},
            {" ; just text", "; just text"},
            {" ; (just text) ", ";"},
            {"text (with something in parentheses)", "text"},
            {"text, with something after comma", "text"},
            {"text; with something after semicolon", "text"},
            {"long text, with something after comma", "long text"},
            {"long text, with something after comma, and a comma", "long text"},
            {"text (with something in parentheses); and something after semicolon", "text"},
            {"text (with something in parentheses, and a comma) and something else", "text  and something else"},
        };

        for (const auto& [target, expected] : cases) {
            UNIT_ASSERT_VALUES_EQUAL(TValue::String(expected),
                                     MusicTitleShorten(ctx, /* globals = */ nullptr, TValue::String(target)));
        }
    }

    Y_UNIT_TEST(PluralizeTag) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const THashMap<TString, TString> samples = {
            {"", ""},
            {"#meter", "#meter"},
            {"test", "test"},
            {"123 #word", "123 #word"},
            {"12", "12"},
            {"1 2 3", "1 2 3"},
            {"12-#meter", "12-#meter"},

            {"масса 1 #kilogram", "масса 1 килограмм"},
            {"высота 2 #meter", "высота 2 метра"},
            {"длина 3 #centimeter", "длина 3 сантиметра"},
            {"вес 4 #gram", "вес 4 грамма"},
            {"продолжительность 5 #minute", "продолжительность 5 минут"},
            {"площадь 6 #square_kilometer", "площадь 6 квадратных километров"},

            {"вес 9 532 #gram", "вес 9 532 грамма"},
            {"вес 1 900 532 #gram", "вес 1 900 532 грамма"},

            {"0 #meter", "0 метров"},
            {"1 #meter", "1 метр"},
            {"2 #meter", "2 метра"},
            {"3 #meter", "3 метра"},
            {"4 #meter", "4 метра"},
            {"5 #meter", "5 метров"},
            {"6 #meter", "6 метров"},
            {"22 #meter", "22 метра"},

            {"12 #meter", "12 метров"},
            {"1 2 #meter", "1 2 метров"},
            {"1|2 #meter", "1|2 метра"},
        };

        for (const auto& [input, target] : samples) {
            UNIT_ASSERT_VALUES_EQUAL_C(TValue::String(target),
                                       PluralizeTag(ctx, /* globals = */ nullptr,
                                                    TValue::String(input), TValue::String("nomn")),
                                       input);
        }

        UNIT_ASSERT_EXCEPTION(PluralizeTag(ctx, nullptr, TValue::None(), TValue::String("nomn")),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(PluralizeTag(ctx, nullptr, TValue::String(""), TValue::None()),
                              TTypeError);
        UNIT_ASSERT_EXCEPTION(PluralizeTag(ctx, nullptr, TValue::String("99999999999999999999999999999 #gram"),
                                           TValue::String("nomn")),
                              TTypeError);
    }

    Y_UNIT_TEST(AddHours) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        const auto date = CreateDateSafe(ctx, nullptr, TValue::Integer(2020), TValue::Integer(7), TValue::Integer(13)); // hours == 0
        const auto changedDate = AddHours(ctx, /* globals = */ nullptr, date, TValue::Integer(13));  // hours == 13

        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "year"), GetAttrLoad(changedDate, "year"));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "month"), GetAttrLoad(changedDate, "month"));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "day"), GetAttrLoad(changedDate, "day"));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "hour"), TValue::Integer(0));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(changedDate, "hour"), TValue::Integer(13));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "second"), GetAttrLoad(date, "second"));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "microsecond"), GetAttrLoad(date, "microsecond"));

        const auto againChangedDate = AddHours(ctx, /* globals = */ nullptr, changedDate, TValue::Integer(13));  // hours == 2, days += 1
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "year"), GetAttrLoad(againChangedDate, "year"));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "month"), GetAttrLoad(againChangedDate, "month"));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "day").GetInteger(), GetAttrLoad(againChangedDate, "day").GetInteger() - 1);
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(againChangedDate, "hour"), TValue::Integer(2));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "second"), GetAttrLoad(date, "second"));
        UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(date, "microsecond"), GetAttrLoad(date, "microsecond"));
    }

    Y_UNIT_TEST(CeilSeconds) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        TVector<std::tuple<TValue, TValue, TValue>> inputAndResults = {
            {
                TValue::Dict({{"hours", TValue::Integer(1)}, {"minutes", TValue::Integer(20)}, {"seconds", TValue::Integer(30)}}),
                TValue::Bool(false),
                TValue::Dict({{"hours", TValue::Integer(1)}, {"minutes", TValue::Integer(21)}, {"seconds", TValue::Integer(0)}}),
            },
            {
                TValue::Dict({{"hours", TValue::Integer(1)}, {"minutes", TValue::Integer(20)}}),
                TValue::Bool(false),
                TValue::Dict({{"hours", TValue::Integer(1)}, {"minutes", TValue::Integer(20)}}),
            },
            {
                TValue::Dict({{"minutes", TValue::Integer(20)}, {"seconds", TValue::Integer(30)}}),
                TValue::Bool(false),
                TValue::Dict({{"minutes", TValue::Integer(20)}, {"seconds", TValue::Integer(30)}}),
            },
            {
                TValue::Dict({{"seconds", TValue::Integer(30)}}),
                TValue::Bool(false),
                TValue::Dict({{"seconds", TValue::Integer(30)}}),
            },
            {
                TValue::Dict({{"minutes", TValue::Integer(59)}}),
                TValue::Bool(false),
                TValue::Dict({{"minutes", TValue::Integer(59)}}),
            },
            {
                TValue::Dict({{"minutes", TValue::Integer(20)}, {"seconds", TValue::Integer(30)}}),
                TValue::Bool(true),
                TValue::Dict({{"hours", TValue::Integer(0)}, {"minutes", TValue::Integer(21)}, {"seconds", TValue::Integer(0)}}),
            },
            {
                TValue::Dict({{"seconds", TValue::Integer(30)}}),
                TValue::Bool(true),
                TValue::Dict({{"hours", TValue::Integer(0)}, {"minutes", TValue::Integer(1)}, {"seconds", TValue::Integer(0)}}),
            },
            {
                TValue::Dict({{"minutes", TValue::Integer(59)}, {"seconds", TValue::Integer(30)}}),
                TValue::Bool(true),
                TValue::Dict({{"hours", TValue::Integer(1)}, {"minutes", TValue::Integer(0)}, {"seconds", TValue::Integer(0)}}),
            },
        };
        for (auto& [units, aggressive, result] : inputAndResults) {
            UNIT_ASSERT_VALUES_EQUAL(CeilSeconds(ctx, /* globals = */ nullptr, units, aggressive), result);
        }
    }

    Y_UNIT_TEST(RenderWeekdaySimple) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};
        TVector<std::pair<cctz::weekday, TValue>> inputAndResults = {
            {cctz::weekday::monday, TValue::String("понедельник")},
            {cctz::weekday::tuesday, TValue::String("вторник")},
            {cctz::weekday::wednesday, TValue::String("среда")},
            {cctz::weekday::thursday, TValue::String("четверг")},
            {cctz::weekday::friday, TValue::String("пятница")},
            {cctz::weekday::saturday, TValue::String("суббота")},
            {cctz::weekday::sunday, TValue::String("воскресенье")}
        };

        for (auto& [input, expectedResult] : inputAndResults) {
            const auto actualResult = RenderWeekdaySimple(ctx, /* globals = */ nullptr, TValue::Integer(static_cast<int>(input)));
            UNIT_ASSERT_VALUES_EQUAL(actualResult, expectedResult);
        }

    }


    Y_UNIT_TEST(RenderWeekdayType) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        TVector<std::pair<TValue, TValue>> inputAndResults = {
            {TValue::Dict({{"weekdays", SeqToList({1})}}), TValue::String("в понедельник")},
            {TValue::Dict({{"weekdays", SeqToList({2, 3})}}), TValue::String("во вторник и среду")},
            {TValue::Dict({{"weekdays", SeqToList({4, 5, 6})}}), TValue::String("в четверг, пятницу и субботу")},
            {TValue::Dict({{"weekdays", SeqToList({1, 2, 3, 4, 5})}}), TValue::String("в будни")},
            {TValue::Dict({{"weekdays", SeqToList({6, 7})}}), TValue::String("в выходные")},
            {TValue::Dict({{"weekdays", SeqToList({7})}, {"repeat", TValue::Bool(true)}}), TValue::String("по воскресеньям")},
            {TValue::Dict({{"weekdays", SeqToList({5, 7})}, {"repeat", TValue::Bool(true)}}), TValue::String("по пятницам и воскресеньям")},
            {TValue::Dict({{"weekdays", SeqToList({2, 3, 5})}, {"repeat", TValue::Bool(true)}}), TValue::String("по вторникам, средам и пятницам")},
            {TValue::Dict({{"weekdays", SeqToList({1, 2, 3, 4, 5})}, {"repeat", TValue::Bool(true)}}), TValue::String("по будням")},
            {TValue::Dict({{"weekdays", SeqToList({6, 7})}, {"repeat", TValue::Bool(true)}}), TValue::String("по выходным")},
            {TValue::Dict({{"weekdays", SeqToList({1, 2, 3, 4, 5, 6, 7})}, {"repeat", TValue::Bool(true)}}), TValue::String("каждый день")}
        };

        for (auto& [input, expectedResult] : inputAndResults) {
            const auto actualResult = RenderWeekdayType(ctx, /* globals = */ nullptr, input);
            UNIT_ASSERT_VALUES_EQUAL(actualResult, expectedResult);
        }
    }

    Y_UNIT_TEST(TimeFormat) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        TVector<std::tuple<TValue, TValue, TValue>> inputAndResults = {
            {
                TValue::Dict({{"hours", TValue::Integer(0)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 12 часов ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("12 часов ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(0)}}),
                TValue::String("gen"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#gen 12 часов ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("12 часов ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(0)}}),
                TValue::String("loc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#loc 12 часах ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("12 часах ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(0)}}),
                TValue::String("ins"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#instr 12 часами ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("12 часами ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(1)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("час ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("час ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(1)}}),
                TValue::String("dat"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("часу ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("часу ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(2)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 2 часа ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("2 часа ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(7)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 7 часов утра"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("7 часов утра"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(9)}, {"period", TValue::String("am")}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 9 часов утра"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("9 часов утра"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(12)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 12 часов дня"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("12 часов дня"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(12)}, {"period", TValue::String("pm")}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 12 часов дня"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("12 часов дня"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(12)}, {"period", TValue::String("am")}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 12 часов ночи"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("12 часов ночи"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(13)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 13 часов"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("13 часов"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(1)}, {"period", TValue::String("pm")}}),
                TValue::String("dat"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#dat 13 часам"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("13 часам"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(2)}, {"minutes", TValue::Integer(30)}, {"period", TValue::String("pm")}}),
                TValue::String("dat"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#dat 14 часам #dat 30 минутам"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("14:30"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(7)}, {"minutes", TValue::Integer(8)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 7 часов #nom 8 минут"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("07:08"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(0)}, {"minutes", TValue::Integer(15)}}),
                TValue::String("nom"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#nom 0 часов #nom 15 минут"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("00:15"))},
                })
            },
        };

        for (auto& [time, cases, expectedResult] : inputAndResults) {
            const auto actualResult = TimeFormat(ctx, /* globals = */ nullptr, time, cases);
            UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(actualResult, "text"), GetAttrLoad(expectedResult, "text"));
            UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(actualResult, "voice"), GetAttrLoad(expectedResult, "voice"));
        }
    }

    Y_UNIT_TEST(RenderUnitsTime) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        TVector<std::tuple<TValue, TValue, TValue>> inputAndResults = {
            {
                TValue::Dict({{"hours", TValue::Integer(1)}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 1 час"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("1 час"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(2)}, {"minutes", TValue::Integer(2)}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 2 часа #acc 2 минуты"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("2 часа 2 минуты"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(5)}, {"seconds", TValue::Integer(5)}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 5 часов #acc 5 секунд"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("5 часов 5 секунд"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(11)}, {"minutes", TValue::Integer(22)},{"seconds", TValue::Integer(41)}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 11 часов #acc 22 минуты #acc 41 секунду"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("11 часов 22 минуты 41 секунду"))},
                })
            },
            {
                TValue::Dict({{"minutes", TValue::Integer(1)}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 1 минуту"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("1 минуту"))},
                })
            },
            {
                TValue::Dict({{"minutes", TValue::Integer(3)}, {"seconds", TValue::Integer(7)}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 3 минуты #acc 7 секунд"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("3 минуты 7 секунд"))},
                })
            },
            {
                TValue::Dict({{"seconds", TValue::Integer(1)}}),
                TValue::String("acc"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#acc 1 секунду"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("1 секунду"))},
                })
            },
            {
                TValue::Dict({{"hours", TValue::Integer(11)}, {"minutes", TValue::Integer(22)},{"seconds", TValue::Integer(41)}}),
                TValue::String("gen"),
                TValue::Dict({
                    {"voice", OnlyVoice(ctx, nullptr, TValue::String("#gen 11 часов #gen 22 минут #gen 41 секунды"))},
                    {"text", OnlyText(ctx, nullptr, TValue::String("11 часов 22 минут 41 секунды"))},
                })
            },
        };

        for (auto& [units, cases, expectedResult] : inputAndResults) {
            const auto actualResult = RenderUnitsTime(ctx, /* globals = */ nullptr, units, cases);
            UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(actualResult, "text"), GetAttrLoad(expectedResult, "text"));
            UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(actualResult, "voice"), GetAttrLoad(expectedResult, "voice"));
        }
    }

    Y_UNIT_TEST(RenderDatetimeRaw) {
        const TEnvironment env;
        TFakeRng rng;
        TCallStack callStack;

        const TCallCtx ctx{env, rng, callStack, RuLanguage};

        NJson::TJsonValue inputAndResultsJson = ReadJson(NResource::Find("/datetime_raw_examples.json"));

        for (auto inputAndResultJson : inputAndResultsJson.GetArray()) {
            auto input = TValue::FromJsonValue(inputAndResultJson.GetArray()[0]);
            auto expectedTextResult = OnlyText(ctx, nullptr, TValue::String(inputAndResultJson.GetArray()[1].GetString()));
            auto expectedVoiceResult = OnlyVoice(ctx, nullptr, TValue::String(inputAndResultJson.GetArray()[2].GetString()));

            const auto actualResult = RenderDatetimeRaw(ctx, /* globals = */ nullptr, input);
            UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(actualResult, "text"), expectedTextResult);
            UNIT_ASSERT_VALUES_EQUAL(GetAttrLoad(actualResult, "voice"), expectedVoiceResult);
        }
    }
}
