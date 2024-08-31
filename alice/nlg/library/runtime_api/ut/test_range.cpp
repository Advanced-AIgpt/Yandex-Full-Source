#include <alice/nlg/library/runtime_api/range.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

namespace {

NAlice::NNlg::TRange RANGE_SIMPLE{0, 3, 1};
NAlice::NNlg::TRange RANGE_DIVISIBLE{1, 9, 2};
NAlice::NNlg::TRange RANGE_DIVISIBLE_LAST{7, 9, 2};
NAlice::NNlg::TRange RANGE_DIVISIBLE_END{9, 9, 2};
NAlice::NNlg::TRange RANGE_NOT_DIVISIBLE{1, 9, 3};
NAlice::NNlg::TRange RANGE_EMPTY_SIMPLE{5, 5, 1};
NAlice::NNlg::TRange RANGE_EMPTY_BY_OVERFLOW{5, -5, 1};
NAlice::NNlg::TRange RANGE_NEG_STEP_SIMPLE{5, -5, -1};
NAlice::NNlg::TRange RANGE_NEG_STEP_STEPPED{-1, -9, -3};

NAlice::NNlg::TRange MIN_RANGE{Min<i64>(), -1, 1};
NAlice::NNlg::TRange MAX_RANGE{0, Max<i64>(), 1};
NAlice::NNlg::TRange MAX_RANGE_STEPPED{0, Max<i64>(), 2};

}  // namespace

Y_UNIT_TEST_SUITE(NlgRange) {
    Y_UNIT_TEST(Size) {
        UNIT_ASSERT_VALUES_EQUAL(3, RANGE_SIMPLE.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(4, RANGE_DIVISIBLE.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(1, RANGE_DIVISIBLE_LAST.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(0, RANGE_DIVISIBLE_END.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(3, RANGE_NOT_DIVISIBLE.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(0, RANGE_EMPTY_SIMPLE.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(0, RANGE_EMPTY_BY_OVERFLOW.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(10, RANGE_NEG_STEP_SIMPLE.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(3, RANGE_NEG_STEP_STEPPED.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(Max<i64>(), MIN_RANGE.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(Max<i64>(), MAX_RANGE.GetSize());
        UNIT_ASSERT_VALUES_EQUAL(Max<i64>() / 2 + 1, MAX_RANGE_STEPPED.GetSize());
    }

    Y_UNIT_TEST(Contains) {
        using NAlice::NNlg::TRange;

        auto checkContains = [](TRange range, const THashSet<i64>& rangeSet) {
            i64 from = range.GetStart() - 50 * range.GetStep();
            i64 to = range.GetStop() + 50 * range.GetStep();
            i64 step = range.GetStep() > 0 ? 1 : -1;
            for (i64 i = from; i < to; i += step) {
                bool contains = rangeSet.contains(i);
                UNIT_ASSERT_VALUES_EQUAL_C(contains, range.Contains(i), "range == " << range << ", i == " << i);
            }
        };

        checkContains(RANGE_SIMPLE, {0, 1, 2});
        checkContains(RANGE_DIVISIBLE, {1, 3, 5, 7});
        checkContains(RANGE_NOT_DIVISIBLE, {1, 4, 7});
        checkContains(RANGE_EMPTY_SIMPLE, {});
        checkContains(RANGE_EMPTY_BY_OVERFLOW, {});
        checkContains(RANGE_NEG_STEP_SIMPLE, {5, 4, 3, 2, 1, 0, -1, -2, -3, -4});
        checkContains(RANGE_NEG_STEP_STEPPED, {-1, -4, -7});

        UNIT_ASSERT(!RANGE_SIMPLE.Contains(Min<i64>()));
        UNIT_ASSERT(!RANGE_SIMPLE.Contains(Max<i64>()));

        UNIT_ASSERT(MIN_RANGE.Contains(Min<i64>()));
        UNIT_ASSERT(MIN_RANGE.Contains(-2));
        UNIT_ASSERT(!MIN_RANGE.Contains(-1));
        UNIT_ASSERT(!MIN_RANGE.Contains(Max<i64>()));

        UNIT_ASSERT(!MAX_RANGE.Contains(Min<i64>()));
        UNIT_ASSERT(!MAX_RANGE.Contains(-1));
        UNIT_ASSERT(MAX_RANGE.Contains(0));
        UNIT_ASSERT(MAX_RANGE.Contains(Max<i64>() - 1));
        UNIT_ASSERT(!MAX_RANGE.Contains(Max<i64>()));

        UNIT_ASSERT(MAX_RANGE_STEPPED.Contains(0));
        UNIT_ASSERT(!MAX_RANGE_STEPPED.Contains(1));
        UNIT_ASSERT(MAX_RANGE_STEPPED.Contains(Max<i64>() - 1));
        UNIT_ASSERT(!MAX_RANGE_STEPPED.Contains(Max<i64>()));
    }

    Y_UNIT_TEST(Subscript) {
        using NAlice::NNlg::TRange;

        auto checkSubscript = [](TRange range, const TVector<i64>& rangeVtor) {
            for (i64 i = 0; i < range.GetSize(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL_C(rangeVtor[i], range[i], "i == " << i);
            }

            UNIT_ASSERT_VALUES_EQUAL(Nothing(), range[range.GetSize()]);
            UNIT_ASSERT_VALUES_EQUAL(Nothing(), range[-range.GetSize() - 1]);
        };

        checkSubscript(RANGE_SIMPLE, {0, 1, 2});
        checkSubscript(RANGE_DIVISIBLE, {1, 3, 5, 7});
        checkSubscript(RANGE_NOT_DIVISIBLE, {1, 4, 7});
        checkSubscript(RANGE_EMPTY_SIMPLE, {});
        checkSubscript(RANGE_EMPTY_BY_OVERFLOW, {});
        checkSubscript(RANGE_NEG_STEP_SIMPLE, {5, 4, 3, 2, 1, 0, -1, -2, -3, -4});
        checkSubscript(RANGE_NEG_STEP_STEPPED, {-1, -4, -7});

        UNIT_ASSERT_VALUES_EQUAL(Min<i64>(), MIN_RANGE[0]);
        UNIT_ASSERT_VALUES_EQUAL(-2, MIN_RANGE[Max<i64>() - 1]);
        UNIT_ASSERT_VALUES_EQUAL(Nothing(), MIN_RANGE[Max<i64>()]);

        UNIT_ASSERT_VALUES_EQUAL(0, MAX_RANGE[0]);
        UNIT_ASSERT_VALUES_EQUAL(Max<i64>() - 1, MAX_RANGE[Max<i64>() - 1]);
        UNIT_ASSERT_VALUES_EQUAL(Nothing(), MAX_RANGE[Max<i64>()]);
    }

    Y_UNIT_TEST(Iteration) {
        using NAlice::NNlg::TRange;

        auto toVector = [](TRange range) {
            TVector<i64> result;
            result.reserve(range.GetSize());

            for (i64 item : range) {
                result.push_back(item);
            }

            return result;
        };

        UNIT_ASSERT_VALUES_EQUAL(TVector<i64>({0, 1, 2}), toVector(RANGE_SIMPLE));
        UNIT_ASSERT_VALUES_EQUAL(TVector<i64>({1, 3, 5, 7}), toVector(RANGE_DIVISIBLE));
        UNIT_ASSERT_VALUES_EQUAL(TVector<i64>({1, 4, 7}), toVector(RANGE_NOT_DIVISIBLE));
        UNIT_ASSERT_VALUES_EQUAL(TVector<i64>({}), toVector(RANGE_EMPTY_SIMPLE));
        UNIT_ASSERT_VALUES_EQUAL(TVector<i64>({}), toVector(RANGE_EMPTY_BY_OVERFLOW));
        UNIT_ASSERT_VALUES_EQUAL(TVector<i64>({5, 4, 3, 2, 1, 0, -1, -2, -3, -4}), toVector(RANGE_NEG_STEP_SIMPLE));
        UNIT_ASSERT_VALUES_EQUAL(TVector<i64>({-1, -4, -7}), toVector(RANGE_NEG_STEP_STEPPED));
    }

    Y_UNIT_TEST(InvalidRanges) {
        using NAlice::NNlg::TRange;
        using NAlice::NNlg::TValueError;

        UNIT_ASSERT_EXCEPTION(TRange(0, 3, 0), TValueError);
        UNIT_ASSERT_EXCEPTION(TRange(Min<i64>(), 0, 1), TValueError);
        UNIT_ASSERT_EXCEPTION(TRange(-1, Max<i64>(), 1), TValueError);
        UNIT_ASSERT_EXCEPTION(TRange(Min<i64>() / 2, -(Min<i64>() / 2), 1), TValueError);
    }
}
