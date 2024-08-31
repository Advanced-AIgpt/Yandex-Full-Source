#include <alice/nlg/library/runtime_api/exceptions.h>
#include <alice/nlg/library/runtime/operators.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/array_size.h>

using namespace NAlice::NNlg;
using namespace NAlice::NNlg::NOperators;

namespace {

void CheckUndefined(TValue (*op)(const TValue&, const TValue&), TStringBuf opName) {
    TValue undefinedCases[] = {
        TValue::Undefined(),
        TValue::Bool(true),
        TValue::Integer(123),
        TValue::Double(456),
        TValue::String("foo"),
        TValue::List({TValue::Integer(1), TValue::Integer(2)}),
        TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
        TValue::Range({0, 5, 1}),
        TValue::None(),
    };

    for (const auto& value : undefinedCases) {
        UNIT_ASSERT_EXCEPTION_C((*op)(TValue::Undefined(), value), TTypeError, opName << ": value = " << Repr(value));
        UNIT_ASSERT_EXCEPTION_C((*op)(value, TValue::Undefined()), TTypeError, opName << ": value = " << Repr(value));
    }
}

} // namespace

Y_UNIT_TEST_SUITE(NlgOperators) {
    Y_UNIT_TEST(Equality) {
        // cases where the types are either equal or upcastable,
        // used to check that we handle these explicit cases correctly
        std::tuple<TValue, TValue, bool> hardCases[] = {
            // bool (upcasts to int, double)
            {TValue::Bool(true), TValue::Bool(false), false},
            {TValue::Bool(true), TValue::Bool(true), true},
            {TValue::Bool(true), TValue::Integer(0), false},
            {TValue::Bool(true), TValue::Integer(1), true},
            {TValue::Bool(true), TValue::Integer(2), false},
            {TValue::Bool(true), TValue::Double(0), false},
            {TValue::Bool(true), TValue::Double(1), true},
            {TValue::Bool(true), TValue::Double(2), false},

            // int (upcasts to double)
            {TValue::Integer(0), TValue::Integer(0), true},
            {TValue::Integer(0), TValue::Integer(-1), false},
            {TValue::Integer(0), TValue::Double(0), true},
            {TValue::Integer(0), TValue::Double(-1), false},

            // double (doesn't upcast)
            {TValue::Double(0), TValue::Double(0), true},
            {TValue::Double(0), TValue::Double(-1), false},

            // string
            {TValue::String("foo"), TValue::String("foo"), true},
            {TValue::String("foo"), TValue::String("bar"), false},

            // list (doesn't upcast, but values may upcast)
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                true,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Double(1), TValue::String("foo"), TValue::Double(3.14)}),
                true,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Bool(true), TValue::String("foo"), TValue::Double(3.14)}),
                true,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Integer(0), TValue::String("foo"), TValue::Double(3.14)}),
                false,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Double(1.1), TValue::String("foo"), TValue::Double(3.14)}),
                false,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Bool(false), TValue::String("foo"), TValue::Double(3.14)}),
                false,
            },

            // dict (doesn't upcast, but values may upcast)
            {
                TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                true,
            },
            {
                TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                TValue::Dict({{"foo", TValue::Bool(true)}, {"bar", TValue::String("baz")}}),
                true,
            },
            {
                TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                TValue::Dict({{"foo", TValue::Double(1)}, {"bar", TValue::String("baz")}}),
                true,
            },
            {
                TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                TValue::Dict({{"foo", TValue::Integer(2)}, {"bar", TValue::String("baz")}}),
                false,
            },
            {
                TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                TValue::Dict({{"awol", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                false,
            },
            {
                TValue::Dict({{"foo", TValue::Integer(1)}, {"bar", TValue::String("baz")}}),
                TValue::Dict({{"foo", TValue::Integer(1)}}),
                false,
            },

            // range (doesn't upcast)
            {TValue::Range({0, 5, 1}), TValue::Range({0, 5, 1}), true},
            {TValue::Range({0, 5, 2}), TValue::Range({0, 6, 2}), true},
            {TValue::Range({0, 5, 1}), TValue::Range({0, 4, 1}), false},

            // none (doesn't upcast)
            {TValue::None(), TValue::None(), true},
            {TValue::None(), TValue::Bool(false), false},
        };

        for (const auto& [a, b, expected] : hardCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(Equals(a, b)), expected,
                                       "Equals: expected = " << expected << ", a = " << Repr(a)
                                                             << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(Equals(b, a)), expected,
                                       "Equals: expected = " << expected << ", a = " << Repr(b)
                                                             << ", b = " << Repr(a));
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(NotEquals(a, b)), !expected,
                                       "NotEquals: expected = " << !expected << ", a = " << Repr(a)
                                                                << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(NotEquals(b, a)), !expected,
                                       "NotEquals: expected = " << !expected << ", a = " << Repr(b)
                                                                << ", b = " << Repr(a));
        }

        // cases where the types and the values are different,
        // used to check that we don't die if given non-matching types
        TValue easyCases[] = {
            TValue::Undefined(),
            TValue::Bool(true),
            TValue::Integer(2),
            TValue::Double(3.14),
            TValue::String("foo"),
            TValue::List({TValue::Integer(1), TValue::Integer(2)}),
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (size_t i = 0; i < Y_ARRAY_SIZE(easyCases); ++i) {
            for (size_t j = 0; j < Y_ARRAY_SIZE(easyCases); ++j) {
                bool expected = i == j;
                const auto& a = easyCases[i];
                const auto& b = easyCases[j];
                UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(Equals(a, b)), expected,
                                           "Equals: expected = " << expected << ", a = " << Repr(a)
                                                                 << ", b = " << Repr(b));
                UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(NotEquals(a, b)), !expected,
                                           "NotEquals: expected = " << !expected << ", a = " << Repr(a)
                                                                    << ", b = " << Repr(b));
            }
        }
    }

    Y_UNIT_TEST(ValueIn) {
        auto ints = TValue::List({TValue::Integer(123), TValue::Integer(456)});
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(123), ints)));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(456), ints)));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(789), ints)));

        auto strings = TValue::List(
            {TValue::String("John"), TValue::String("Paul"), TValue::String("George"), TValue::String("Ringo")});
        UNIT_ASSERT(TruthValue(ValueIn(TValue::String("George"), strings)));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::String("Bill"), strings)));

        auto dict = TValue::Dict({{"foo", TValue::Integer(123)}, {"bar", TValue::None()}});
        UNIT_ASSERT(TruthValue(ValueIn(TValue::String("foo"), dict)));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::String("fool"), dict)));

        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(0), TValue::Range({0, 5, 1}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(4), TValue::Range({0, 5, 1}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(5), TValue::Range({0, 5, 1}))));

        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(0), TValue::Range({1, 5, 1}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(1), TValue::Range({1, 5, 1}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(4), TValue::Range({1, 5, 1}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(5), TValue::Range({1, 5, 1}))));

        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(0), TValue::Range({1, 5, 2}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(1), TValue::Range({1, 5, 2}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(2), TValue::Range({1, 5, 2}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(3), TValue::Range({1, 5, 2}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(4), TValue::Range({1, 5, 2}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(5), TValue::Range({1, 5, 2}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(6), TValue::Range({1, 5, 2}))));

        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(0), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(1), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(2), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(3), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(4), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(5), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(6), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(TruthValue(ValueIn(TValue::Integer(7), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(8), TValue::Range({1, 9, 3}))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(9), TValue::Range({1, 9, 3}))));

        UNIT_ASSERT(!TruthValue(ValueIn(TValue::Integer(123), TValue::Integer(123))));

        UNIT_ASSERT(TruthValue(ValueIn(TValue::String("test"), TValue::String("123 test 456"))));
        UNIT_ASSERT(!TruthValue(ValueIn(TValue::String("needle"), TValue::String("haystack"))));
    }

    Y_UNIT_TEST(Comparisons) {
        // cases where the types are either equal or upcastable,
        // used to check that we handle these explicit cases correctly
        std::tuple<TValue, TValue, bool> hardCases[] = {
            // bool (upcasts to int, double)
            {TValue::Bool(true), TValue::Bool(false), false},
            {TValue::Bool(false), TValue::Bool(true), true},
            {TValue::Bool(true), TValue::Bool(true), false},
            {TValue::Bool(true), TValue::Integer(0), false},
            {TValue::Bool(true), TValue::Integer(1), false},
            {TValue::Bool(true), TValue::Integer(2), true},
            {TValue::Bool(true), TValue::Double(0), false},
            {TValue::Bool(true), TValue::Double(1), false},
            {TValue::Bool(true), TValue::Double(2), true},

            // int (upcasts to double)
            {TValue::Integer(0), TValue::Integer(0), false},
            {TValue::Integer(0), TValue::Integer(-1), false},
            {TValue::Integer(0), TValue::Integer(1), true},
            {TValue::Integer(0), TValue::Double(0), false},
            {TValue::Integer(0), TValue::Double(-1), false},
            {TValue::Integer(0), TValue::Double(1), true},

            // double (doesn't upcast)
            {TValue::Double(0), TValue::Double(0), false},
            {TValue::Double(0), TValue::Double(-1), false},
            {TValue::Double(0), TValue::Double(1), true},

            // string
            {TValue::String("foo"), TValue::String("foo"), false},
            {TValue::String("foo"), TValue::String("fooo"), true},
            {TValue::String("fooo"), TValue::String("foo"), false},
            {TValue::String("foo"), TValue::String("bar"), false},
            {TValue::String("bar"), TValue::String("baz"), true},

            // list (doesn't upcast, but values may upcast)
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                false,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Double(1), TValue::String("foo"), TValue::Double(3.14)}),
                false,
            },
            {
                TValue::List({TValue::Double(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                false,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo")}),
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                true,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Integer(1), TValue::String("foo")}),
                false,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.15)}),
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                false,
            },
            {
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.14)}),
                TValue::List({TValue::Integer(1), TValue::String("foo"), TValue::Double(3.15)}),
                true,
            },
        };

        for (const auto& [a, b, expected] : hardCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(Less(a, b)), expected,
                                       "Less: expected = " << expected << ", a = " << Repr(a) << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(Greater(b, a)), expected,
                                       "Greater: expected = " << expected << ", a = " << Repr(b)
                                                              << ", b = " << Repr(a));
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(GreaterEq(a, b)), !expected,
                                       "GreaterEq: expected = " << !expected << ", a = " << Repr(a)
                                                                << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(TruthValue(LessEq(b, a)), !expected,
                                       "LessEq: expected = " << !expected << ", a = " << Repr(b)
                                                             << ", b = " << Repr(a));
        }

        // check that comparing unrelated types throws
        TValue negativeCases[] = {
            TValue::Undefined(),
            TValue::Integer(2),
            TValue::String("foo"),
            TValue::List({TValue::Integer(1), TValue::Integer(2)}),
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (size_t i = 0; i < Y_ARRAY_SIZE(negativeCases); ++i) {
            for (size_t j = 0; j < Y_ARRAY_SIZE(negativeCases); ++j) {
                if (i == j) {
                    continue;
                }
                const auto& a = negativeCases[i];
                const auto& b = negativeCases[j];
                UNIT_ASSERT_EXCEPTION_C(Less(a, b), TTypeError, "Less: a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(LessEq(a, b), TTypeError, "LessEq: a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(Greater(a, b), TTypeError, "Greater: a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(GreaterEq(a, b), TTypeError,
                                        "GreaterEq: a = " << Repr(a) << ", b = " << Repr(b));
            }
        }

        UNIT_ASSERT_EXCEPTION(Less(TValue::Dict(), TValue::Dict()), TTypeError);
        UNIT_ASSERT_EXCEPTION(Less(TValue::Range({0, 5, 1}), TValue::Range({0, 5, 1})), TTypeError);
        UNIT_ASSERT_EXCEPTION(Less(TValue::None(), TValue::None()), TTypeError);
    }

    Y_UNIT_TEST(UnaryMinus) {
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(-5), UnaryMinus(TValue::Integer(5)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Double(-5.0), UnaryMinus(TValue::Double(5.0)));

        UNIT_ASSERT_EXCEPTION(UnaryMinus(TValue::Undefined()), TTypeError);
        UNIT_ASSERT_EXCEPTION(UnaryMinus(TValue::String("foo")), TTypeError);
    }

    Y_UNIT_TEST(UnaryPlus) {
        UNIT_ASSERT_VALUES_EQUAL(TValue::Integer(5), UnaryPlus(TValue::Integer(5)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Double(5.0), UnaryPlus(TValue::Double(5.0)));

        UNIT_ASSERT_EXCEPTION(UnaryPlus(TValue::Undefined()), TTypeError);
        UNIT_ASSERT_EXCEPTION(UnaryPlus(TValue::String("foo")), TTypeError);
    }

    Y_UNIT_TEST(UnaryNot) {
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(false), UnaryNot(TValue::Integer(5)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(true), UnaryNot(TValue::Integer(0)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(false), UnaryNot(TValue::Double(5.0)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(true), UnaryNot(TValue::Double(0)));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(false), UnaryNot(TValue::String("asdf")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(true), UnaryNot(TValue::String("")));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(false), UnaryNot(TValue::Range({0, 5, 1})));
        UNIT_ASSERT_VALUES_EQUAL(TValue::Bool(true), UnaryNot(TValue::Range({5, 5, 1})));
    }

    Y_UNIT_TEST(Mul) {
        std::tuple<TValue, TValue, TValue> positiveCases[] = {
            // numeric cases
            {TValue::Bool(false), TValue::Bool(true), TValue::Integer(0)},
            {TValue::Bool(true), TValue::Bool(true), TValue::Integer(1)},
            {TValue::Bool(false), TValue::Integer(123), TValue::Integer(0)},
            {TValue::Bool(true), TValue::Integer(123), TValue::Integer(123)},
            {TValue::Integer(2), TValue::Integer(4), TValue::Integer(8)},
            {TValue::Integer(2), TValue::Double(4), TValue::Double(8)},
            {TValue::Integer(2), TValue::Double(4), TValue::Double(8)},
            {TValue::Double(2), TValue::Double(4), TValue::Double(8)},
            {TValue::Bool(true), TValue::Double(4), TValue::Double(4)},

            // replication cases
            {TValue::Bool(false), TValue::String("asdf"), TValue::String("")},
            {TValue::Integer(0), TValue::String("asdf"), TValue::String("")},
            {TValue::Integer(-1), TValue::String("asdf"), TValue::String("")},
            {TValue::Bool(true), TValue::String("asdf"), TValue::String("asdf")},
            {TValue::Integer(2), TValue::String("asdf"), TValue::String("asdfasdf")},

            {TValue::Bool(false), TValue::List({TValue::Integer(1)}), TValue::List()},
            {TValue::Integer(0), TValue::List({TValue::Integer(1)}), TValue::List()},
            {TValue::Integer(-1), TValue::List({TValue::Integer(1)}), TValue::List()},
            {TValue::Bool(true), TValue::List({TValue::Integer(1)}), TValue::List({TValue::Integer(1)})},
            {TValue::Integer(2), TValue::List({TValue::Integer(1)}),
             TValue::List({TValue::Integer(1), TValue::Integer(1)})},
        };

        for (const auto& [a, b, ab] : positiveCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(Mul(a, b), ab, "a = " << Repr(a) << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(Mul(b, a), ab, "a = " << Repr(b) << ", b = " << Repr(a));
        }

        // check that multiplying non-numeric types throws
        TValue negativeCases[] = {
            TValue::Double(3.14), // double is numeric but it can't be used for replication
            TValue::String("foo"),
            TValue::List({TValue::Integer(1), TValue::Integer(2)}),
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (size_t i = 0; i < Y_ARRAY_SIZE(negativeCases); ++i) {
            for (size_t j = 0; j < Y_ARRAY_SIZE(negativeCases); ++j) {
                if (i == j) {
                    continue;
                }
                const auto& a = negativeCases[i];
                const auto& b = negativeCases[j];
                UNIT_ASSERT_EXCEPTION_C(Mul(a, b), TTypeError, "a = " << Repr(a) << ", b = " << Repr(b));
            }
        }

        // check that stuff that can't be replicated can't indeed be replicated
        TValue negativeReplicationCases[] = {
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (const auto& a : negativeReplicationCases) {
            UNIT_ASSERT_EXCEPTION_C(Mul(a, TValue::Bool(true)), TTypeError, "a = " << Repr(a));
            UNIT_ASSERT_EXCEPTION_C(Mul(a, TValue::Integer(123)), TTypeError, "a = " << Repr(a));
        }

        CheckUndefined(&Mul, "Mul");
    }

    // tests TrueDiv, FloorDiv, Mod and Pow
    Y_UNIT_TEST(DivModPow) {
        using V = TValue;
        std::tuple<TValue, TValue, TValue, TValue, TValue, TValue> positiveCases[] = {
            // a,  b,  a / b,  a // b,  a % b,  a ** b
            {V::Integer(4), V::Integer(4), V::Double(1), V::Integer(1), V::Integer(0), V::Integer(256)},
            {V::Integer(5), V::Integer(4), V::Double(1.25), V::Integer(1), V::Integer(1), V::Integer(625)},
            {V::Integer(6), V::Integer(4), V::Double(1.5), V::Integer(1), V::Integer(2), V::Integer(1296)},
            {V::Integer(7), V::Integer(4), V::Double(1.75), V::Integer(1), V::Integer(3), V::Integer(2401)},
            {V::Integer(-4), V::Integer(4), V::Double(-1), V::Integer(-1), V::Integer(0), V::Integer(256)},
            {V::Integer(-5), V::Integer(4), V::Double(-1.25), V::Integer(-2), V::Integer(3), V::Integer(625)},
            {V::Integer(-6), V::Integer(4), V::Double(-1.5), V::Integer(-2), V::Integer(2), V::Integer(1296)},
            {V::Integer(-7), V::Integer(4), V::Double(-1.75), V::Integer(-2), V::Integer(1), V::Integer(2401)},
            {V::Integer(4), V::Integer(-4), V::Double(-1), V::Integer(-1), V::Integer(0), V::Double(1.0 / 256)},
            {V::Integer(5), V::Integer(-4), V::Double(-1.25), V::Integer(-2), V::Integer(-3), V::Double(1.0 / 625)},
            {V::Integer(6), V::Integer(-4), V::Double(-1.5), V::Integer(-2), V::Integer(-2), V::Double(1.0 / 1296)},
            {V::Integer(7), V::Integer(-4), V::Double(-1.75), V::Integer(-2), V::Integer(-1), V::Double(1.0 / 2401)},
            {V::Integer(-4), V::Integer(-4), V::Double(1), V::Integer(1), V::Integer(0), V::Double(1.0 / 256)},
            {V::Integer(-5), V::Integer(-4), V::Double(1.25), V::Integer(1), V::Integer(-1), V::Double(1.0 / 625)},
            {V::Integer(-6), V::Integer(-4), V::Double(1.5), V::Integer(1), V::Integer(-2), V::Double(1.0 / 1296)},
            {V::Integer(-7), V::Integer(-4), V::Double(1.75), V::Integer(1), V::Integer(-3), V::Double(1.0 / 2401)},

            {V::Bool(false), V::Integer(1), V::Double(0), V::Integer(0), V::Integer(0), V::Integer(0)},
            {V::Integer(0), V::Bool(true), V::Double(0), V::Integer(0), V::Integer(0), V::Integer(0)},

            {V::Double(4), V::Integer(4), V::Double(1), V::Double(1), V::Double(0), V::Double(256)},
            {V::Double(5), V::Integer(4), V::Double(1.25), V::Double(1), V::Double(1), V::Double(625)},
            {V::Double(6), V::Integer(4), V::Double(1.5), V::Double(1), V::Double(2), V::Double(1296)},
            {V::Double(7), V::Integer(4), V::Double(1.75), V::Double(1), V::Double(3), V::Double(2401)},
            {V::Double(-4), V::Integer(4), V::Double(-1), V::Double(-1), V::Double(0), V::Double(256)},
            {V::Double(-5), V::Integer(4), V::Double(-1.25), V::Double(-2), V::Double(3), V::Double(625)},
            {V::Double(-6), V::Integer(4), V::Double(-1.5), V::Double(-2), V::Double(2), V::Double(1296)},
            {V::Double(-7), V::Integer(4), V::Double(-1.75), V::Double(-2), V::Double(1), V::Double(2401)},
            {V::Integer(4), V::Double(-4), V::Double(-1), V::Double(-1), V::Double(0), V::Double(1.0 / 256)},
            {V::Integer(5), V::Double(-4), V::Double(-1.25), V::Double(-2), V::Double(-3), V::Double(1.0 / 625)},
            {V::Integer(6), V::Double(-4), V::Double(-1.5), V::Double(-2), V::Double(-2), V::Double(1.0 / 1296)},
            {V::Integer(7), V::Double(-4), V::Double(-1.75), V::Double(-2), V::Double(-1), V::Double(1.0 / 2401)},
            {V::Integer(-4), V::Double(-4), V::Double(1), V::Double(1), V::Double(0), V::Double(1.0 / 256)},
            {V::Integer(-5), V::Double(-4), V::Double(1.25), V::Double(1), V::Double(-1), V::Double(1.0 / 625)},
            {V::Integer(-6), V::Double(-4), V::Double(1.5), V::Double(1), V::Double(-2), V::Double(1.0 / 1296)},
            {V::Integer(-7), V::Double(-4), V::Double(1.75), V::Double(1), V::Double(-3), V::Double(1.0 / 2401)},
        };

        for (const auto& [a, b, trueDiv, floorDiv, mod, pow] : positiveCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(TrueDiv(a, b), trueDiv, "a = " << Repr(a) << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(FloorDiv(a, b), floorDiv, "a = " << Repr(a) << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(Mod(a, b), mod, "a = " << Repr(a) << ", b = " << Repr(b));
            UNIT_ASSERT_VALUES_EQUAL_C(Pow(a, b), pow, "a = " << Repr(a) << ", b = " << Repr(b));
        }

        // check zero division
        for (const auto& a : {V::Bool(true), V::Integer(1), V::Double(1)}) {
            for (const auto& b : {V::Bool(false), V::Integer(0), V::Double(0)}) {
                UNIT_ASSERT_EXCEPTION_C(TrueDiv(a, b), TZeroDivisionError, "a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(FloorDiv(a, b), TZeroDivisionError, "a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(Mod(a, b), TZeroDivisionError, "a = " << Repr(a) << ", b = " << Repr(b));
            }
        }

        // check incompatible types
        TValue negativeCases[] = {
            TValue::Double(3.14),
            TValue::String("foo"),
            TValue::List({TValue::Integer(1), TValue::Integer(2)}),
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (size_t i = 0; i < Y_ARRAY_SIZE(negativeCases); ++i) {
            for (size_t j = 0; j < Y_ARRAY_SIZE(negativeCases); ++j) {
                if (i == j) {
                    continue;
                }

                const auto& a = negativeCases[i];
                const auto& b = negativeCases[j];

                UNIT_ASSERT_EXCEPTION_C(TrueDiv(a, b), TTypeError, "a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(FloorDiv(a, b), TTypeError, "a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(Mod(a, b), TTypeError, "a = " << Repr(a) << ", b = " << Repr(b));
                UNIT_ASSERT_EXCEPTION_C(Pow(a, b), TTypeError, "a = " << Repr(a) << ", b = " << Repr(b));
            }
        }

        UNIT_ASSERT_EXCEPTION(TrueDiv(TValue::String(""), TValue::String("")), TTypeError);
        UNIT_ASSERT_EXCEPTION(TrueDiv(TValue::List(), TValue::List()), TTypeError);
        UNIT_ASSERT_EXCEPTION(TrueDiv(TValue::Dict(), TValue::Dict()), TTypeError);
        UNIT_ASSERT_EXCEPTION(TrueDiv(TValue::Range({0, 5, 1}), TValue::Range({0, 5, 1})), TTypeError);
        UNIT_ASSERT_EXCEPTION(TrueDiv(TValue::None(), TValue::None()), TTypeError);

        CheckUndefined(TrueDiv, "TrueDiv");
        CheckUndefined(FloorDiv, "FloorDiv");
        CheckUndefined(Mod, "Mod");
        CheckUndefined(Pow, "Pow");
    }

    Y_UNIT_TEST(Add) {
        std::tuple<TValue, TValue, TValue> positiveCases[] = {
            // numeric cases
            {TValue::Bool(false), TValue::Bool(true), TValue::Integer(1)},
            {TValue::Bool(true), TValue::Bool(true), TValue::Integer(2)},
            {TValue::Bool(false), TValue::Integer(123), TValue::Integer(123)},
            {TValue::Bool(true), TValue::Integer(123), TValue::Integer(124)},
            {TValue::Integer(2), TValue::Integer(4), TValue::Integer(6)},
            {TValue::Integer(2), TValue::Double(4), TValue::Double(6)},
            {TValue::Integer(2), TValue::Double(4), TValue::Double(6)},
            {TValue::Double(2), TValue::Double(4), TValue::Double(6)},
            {TValue::Bool(true), TValue::Double(4), TValue::Double(5)},

            // concatenation cases
            {TValue::String("asdf"), TValue::String("jkl"), TValue::String("asdfjkl")},
            {TValue::List({TValue::Integer(1)}), TValue::List({TValue::Integer(2)}),
             TValue::List({TValue::Integer(1), TValue::Integer(2)})},
        };

        for (const auto& [a, b, ab] : positiveCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(Add(a, b), ab, "a = " << Repr(a) << ", b = " << Repr(b));
        }

        // check that adding incompatible types throws
        TValue negativeCases[] = {
            TValue::Double(3.14),
            TValue::String("foo"),
            TValue::List({TValue::Integer(1), TValue::Integer(2)}),
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (size_t i = 0; i < Y_ARRAY_SIZE(negativeCases); ++i) {
            for (size_t j = 0; j < Y_ARRAY_SIZE(negativeCases); ++j) {
                if (i == j) {
                    continue;
                }
                const auto& a = negativeCases[i];
                const auto& b = negativeCases[j];
                UNIT_ASSERT_EXCEPTION_C(Add(a, b), TTypeError, "a = " << Repr(a) << ", b = " << Repr(b));
            }
        }

        // check that stuff that can't be concatenated can't indeed be concatenated
        TValue negativeReplicationCases[] = {
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (const auto& a : negativeReplicationCases) {
            UNIT_ASSERT_EXCEPTION_C(Add(a, a), TTypeError, "a = " << Repr(a));
            UNIT_ASSERT_EXCEPTION_C(Add(a, a), TTypeError, "a = " << Repr(a));
        }

        CheckUndefined(&Add, "Add");
    }

    Y_UNIT_TEST(Sub) {
        std::tuple<TValue, TValue, TValue> positiveCases[] = {
            // numeric cases
            {TValue::Bool(false), TValue::Bool(true), TValue::Integer(-1)},
            {TValue::Bool(true), TValue::Bool(true), TValue::Integer(0)},
            {TValue::Bool(false), TValue::Integer(123), TValue::Integer(-123)},
            {TValue::Bool(true), TValue::Integer(123), TValue::Integer(-122)},
            {TValue::Integer(2), TValue::Integer(4), TValue::Integer(-2)},
            {TValue::Integer(2), TValue::Double(4), TValue::Double(-2)},
            {TValue::Integer(2), TValue::Double(4), TValue::Double(-2)},
            {TValue::Double(2), TValue::Double(4), TValue::Double(-2)},
            {TValue::Bool(true), TValue::Double(4), TValue::Double(-3)},
        };

        for (const auto& [a, b, ab] : positiveCases) {
            UNIT_ASSERT_VALUES_EQUAL_C(Sub(a, b), ab, "a = " << Repr(a) << ", b = " << Repr(b));
        }

        // check that subtracting incompatible types throws
        TValue negativeCases[] = {
            TValue::String("foo"),
            TValue::List({TValue::Integer(1), TValue::Integer(2)}),
            TValue::Dict({{"foo", TValue::String("bar")}, {"baz", TValue::Integer(123)}}),
            TValue::Range({0, 5, 1}),
            TValue::None(),
        };

        for (size_t i = 0; i < Y_ARRAY_SIZE(negativeCases); ++i) {
            for (size_t j = 0; j < Y_ARRAY_SIZE(negativeCases); ++j) {
                const auto& a = negativeCases[i];
                const auto& b = negativeCases[j];
                UNIT_ASSERT_EXCEPTION_C(Sub(a, b), TTypeError, "a = " << Repr(a) << ", b = " << Repr(b));
            }
        }

        UNIT_ASSERT_EXCEPTION(Sub(TValue::Double(3.14), TValue::String("123")), TTypeError);
        UNIT_ASSERT_EXCEPTION(Sub(TValue::Integer(1), TValue::List()), TTypeError);

        CheckUndefined(&Sub, "Sub");
    }

    Y_UNIT_TEST(And) {
        class TMyException {};

        auto lazyTrue = []() { return TValue::Integer(123); };
        auto lazyFalse = []() { return TValue::String(""); };
        auto lazyBomb = []() -> TValue { throw TMyException(); };

        UNIT_ASSERT_VALUES_EQUAL(lazyFalse(), And(lazyFalse(), lazyFalse));
        UNIT_ASSERT_VALUES_EQUAL(lazyFalse(), And(lazyTrue(), lazyFalse));
        UNIT_ASSERT_VALUES_EQUAL(lazyFalse(), And(lazyFalse(), lazyTrue));
        UNIT_ASSERT_VALUES_EQUAL(lazyTrue(), And(lazyTrue(), lazyTrue));

        UNIT_ASSERT_VALUES_EQUAL(lazyFalse(), And(lazyFalse(), lazyBomb));
        UNIT_ASSERT_EXCEPTION(And(lazyTrue(), lazyBomb), TMyException);
    }

    Y_UNIT_TEST(Or) {
        class TMyException {};

        auto lazyTrue = []() { return TValue::Integer(123); };
        auto lazyFalse = []() { return TValue::String(""); };
        auto lazyBomb = []() -> TValue { throw TMyException(); };

        UNIT_ASSERT_VALUES_EQUAL(lazyFalse(), Or(lazyFalse(), lazyFalse));
        UNIT_ASSERT_VALUES_EQUAL(lazyTrue(), Or(lazyTrue(), lazyFalse));
        UNIT_ASSERT_VALUES_EQUAL(lazyTrue(), Or(lazyFalse(), lazyTrue));
        UNIT_ASSERT_VALUES_EQUAL(lazyTrue(), Or(lazyTrue(), lazyTrue));

        UNIT_ASSERT_VALUES_EQUAL(lazyTrue(), Or(lazyTrue(), lazyBomb));
        UNIT_ASSERT_EXCEPTION(Or(lazyFalse(), lazyBomb), TMyException);
    }

    Y_UNIT_TEST(Slice) {
        auto makeList = [](auto... xs) {
            return TValue::List({TValue::Integer(xs)...});
        };

        auto iv = [](i64 value) {
            return TValue::Integer(value);
        };

        auto makeStr = [](TStringBuf value) {
            return TValue::String(value);
        };

        auto none = TValue::None();

        auto list = makeList(1, 2, 3);
        auto string = makeStr("щ+Σ");

        UNIT_ASSERT_VALUES_EQUAL(list, SliceList(list, iv(0), iv(10), iv(1)));
        UNIT_ASSERT_VALUES_EQUAL(string, SliceList(string, iv(0), iv(10), iv(1)));

        UNIT_ASSERT_VALUES_EQUAL(makeList(1, 2), SliceList(list, iv(0), iv(-1), iv(1)));
        UNIT_ASSERT_VALUES_EQUAL(makeList(1), SliceList(list, iv(0), iv(-2), iv(1)));
        UNIT_ASSERT_VALUES_EQUAL(makeList(), SliceList(list, iv(0), iv(-3), iv(1)));

        UNIT_ASSERT_VALUES_EQUAL(makeStr("щ+"), SliceList(string, iv(0), iv(-1), iv(1)));
        UNIT_ASSERT_VALUES_EQUAL(makeStr("щ"), SliceList(string, iv(0), iv(-2), iv(1)));
        UNIT_ASSERT_VALUES_EQUAL(makeStr(""), SliceList(string, iv(0), iv(-3), iv(1)));

        UNIT_ASSERT_VALUES_EQUAL(makeList(3, 2), SliceList(list, iv(-1), iv(0), iv(-1)));
        UNIT_ASSERT_VALUES_EQUAL(makeList(), SliceList(list, iv(-100), iv(0), iv(-1)));
        UNIT_ASSERT_VALUES_EQUAL(makeList(), SliceList(list, iv(-1), iv(0), iv(1)));

        UNIT_ASSERT_EXCEPTION(SliceList(string, iv(-1), iv(0), iv(-1)), TValueError);
        UNIT_ASSERT_VALUES_EQUAL(makeStr(""), SliceList(string, iv(-1), iv(0), iv(1)));

        UNIT_ASSERT_VALUES_EQUAL(makeList(3, 1), SliceList(list, iv(-1), iv(-5), iv(-2)));
        UNIT_ASSERT_VALUES_EQUAL(makeList(), SliceList(list, iv(-1), iv(-5), iv(2)));

        UNIT_ASSERT_VALUES_EQUAL(makeList(1, 2), SliceList(list, none, iv(-1), none));
        UNIT_ASSERT_VALUES_EQUAL(makeList(2, 3), SliceList(list, iv(1), none, none));
        UNIT_ASSERT_VALUES_EQUAL(makeList(), SliceList(list, iv(-1), iv(0), none));

        UNIT_ASSERT_VALUES_EQUAL(makeStr("щ+"), SliceList(string, none, iv(-1), none));
        UNIT_ASSERT_VALUES_EQUAL(makeStr("+Σ"), SliceList(string, iv(1), none, none));
        UNIT_ASSERT_VALUES_EQUAL(makeStr(""), SliceList(string, iv(-1), iv(0), none));
    }
}
