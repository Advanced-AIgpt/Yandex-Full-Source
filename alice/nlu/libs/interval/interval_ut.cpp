#include "interval.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/hash.h>

using namespace NNlu;

Y_UNIT_TEST_SUITE(TInterval) {

    Y_UNIT_TEST(Valid) {
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Valid(), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 10}).Valid(), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 9}).Valid(), false);
    }

    Y_UNIT_TEST(Length) {
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Length(), 5);
        UNIT_ASSERT_EQUAL(TInterval({10, 10}).Length(), 0);
    }

    Y_UNIT_TEST(Empty) {
        UNIT_ASSERT_EQUAL(TInterval({0, 5}).Empty(), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 10}).Empty(), true);
    }

    void TestOverlaps(const TInterval& interval1, const TInterval& interval2, bool expected) {
        UNIT_ASSERT_EQUAL(interval1.Overlaps(interval2), expected);
        UNIT_ASSERT_EQUAL(interval2.Overlaps(interval1), expected);
    }

    Y_UNIT_TEST(Overlaps) {
        TestOverlaps({10, 10}, {5, 5}, false);
        TestOverlaps({10, 10}, {10, 10}, false);
        TestOverlaps({10, 15}, {5, 5}, false);
        TestOverlaps({10, 15}, {5, 7}, false);
        TestOverlaps({10, 15}, {5, 10}, false);
        TestOverlaps({10, 15}, {5, 11}, true);
        TestOverlaps({10, 15}, {5, 15}, true);
        TestOverlaps({10, 15}, {5, 20}, true);
        TestOverlaps({10, 15}, {12, 12}, false);
        TestOverlaps({10, 15}, {12, 13}, true);
        TestOverlaps({10, 15}, {12, 15}, true);
        TestOverlaps({10, 15}, {12, 20}, true);
        TestOverlaps({10, 15}, {15, 15}, false);
        TestOverlaps({10, 15}, {15, 20}, false);
        TestOverlaps({10, 15}, {20, 20}, false);
        TestOverlaps({10, 15}, {20, 21}, false);
    }

    Y_UNIT_TEST(ContainsPoint) {
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains(9), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains(10), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains(14), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains(15), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 10}).Contains(10), false);
    }

    Y_UNIT_TEST(ContainsEmpty) {
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({9, 9}), true);
        UNIT_ASSERT_EQUAL(TInterval({15, 15}).Contains({9, 9}), true);
        UNIT_ASSERT_EQUAL(TInterval({15, 15}).Contains({15, 16}), false);
    }

    Y_UNIT_TEST(ContainsInterval) {
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({8, 9}), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({8, 10}), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({8, 12}), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({8, 20}), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({10, 12}), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({10, 15}), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({10, 16}), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({11, 12}), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({11, 15}), true);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({11, 16}), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({15, 16}), false);
        UNIT_ASSERT_EQUAL(TInterval({10, 15}).Contains({20, 21}), false);
    }

    void TestIntersection(const TInterval& interval1, const TInterval& interval2, const TInterval& expected) {
        UNIT_ASSERT_EQUAL(interval1 & interval2, expected);
        UNIT_ASSERT_EQUAL(interval2 & interval1, expected);
        TInterval actual = interval1;
        UNIT_ASSERT_EQUAL(actual &= interval2, expected);
        UNIT_ASSERT_EQUAL(actual, expected);
        actual = interval2;
        UNIT_ASSERT_EQUAL(actual &= interval1, expected);
        UNIT_ASSERT_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(Intersection) {
        TestIntersection({10, 10}, {5, 5},   {0, 0});
        TestIntersection({10, 10}, {10, 10}, {0, 0});
        TestIntersection({10, 15}, {5, 5},   {0, 0});
        TestIntersection({10, 15}, {5, 7},   {0, 0});
        TestIntersection({10, 15}, {5, 10},  {0, 0});
        TestIntersection({10, 15}, {5, 11},  {10, 11});
        TestIntersection({10, 15}, {5, 15},  {10, 15});
        TestIntersection({10, 15}, {5, 20},  {10, 15});
        TestIntersection({10, 15}, {12, 12}, {0, 0});
        TestIntersection({10, 15}, {12, 13}, {12, 13});
        TestIntersection({10, 15}, {12, 15}, {12, 15});
        TestIntersection({10, 15}, {12, 20}, {12, 15});
        TestIntersection({10, 15}, {15, 15}, {0, 0});
        TestIntersection({10, 15}, {15, 20}, {0, 0});
        TestIntersection({10, 15}, {20, 20}, {0, 0});
        TestIntersection({10, 15}, {20, 21}, {0, 0});
    }

    void TestUnion(const TInterval& interval1, const TInterval& interval2, const TInterval& expected) {
        UNIT_ASSERT_EQUAL(interval1 | interval2, expected);
        UNIT_ASSERT_EQUAL(interval2 | interval1, expected);
        TInterval actual = interval1;
        UNIT_ASSERT_EQUAL(actual |= interval2, expected);
        UNIT_ASSERT_EQUAL(actual, expected);
        actual = interval2;
        UNIT_ASSERT_EQUAL(actual |= interval1, expected);
        UNIT_ASSERT_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(Union) {
        TestUnion({10, 10}, {5, 5},   {0, 0});
        TestUnion({10, 10}, {10, 10}, {0, 0});
        TestUnion({10, 15}, {5, 5},   {10, 15});
        TestUnion({10, 15}, {5, 7},   {5, 15});
        TestUnion({10, 15}, {5, 10},  {5, 15});
        TestUnion({10, 15}, {5, 11},  {5, 15});
        TestUnion({10, 15}, {5, 15},  {5, 15});
        TestUnion({10, 15}, {5, 20},  {5, 20});
        TestUnion({10, 15}, {12, 12}, {10, 15});
        TestUnion({10, 15}, {12, 13}, {10, 15});
        TestUnion({10, 15}, {12, 15}, {10, 15});
        TestUnion({10, 15}, {12, 20}, {10, 20});
        TestUnion({10, 15}, {15, 15}, {10, 15});
        TestUnion({10, 15}, {15, 20}, {10, 20});
        TestUnion({10, 15}, {20, 20}, {10, 15});
        TestUnion({10, 15}, {20, 21}, {10, 21});
    }

    void TestJaccardIndex(const TInterval& interval1, const TInterval& interval2, double expected) {
        const double epsilon = 1.e-20;
        UNIT_ASSERT(abs(JaccardIndex(interval1, interval2) - expected) < epsilon);
        UNIT_ASSERT(abs(JaccardIndex(interval2, interval1) - expected) < epsilon);
    }

    Y_UNIT_TEST(JaccardIndex) {
        TestJaccardIndex({10, 10}, {5, 5},   0.);
        TestJaccardIndex({10, 10}, {10, 10}, 0.);
        TestJaccardIndex({10, 15}, {5, 5},   0.);
        TestJaccardIndex({10, 15}, {5, 7},   0.);
        TestJaccardIndex({10, 15}, {5, 10},  0.);
        TestJaccardIndex({10, 15}, {5, 11},  0.1);
        TestJaccardIndex({10, 15}, {5, 15},  0.5);
        TestJaccardIndex({10, 15}, {5, 25},  0.25);
        TestJaccardIndex({10, 15}, {12, 12}, 0.);
        TestJaccardIndex({10, 15}, {12, 13}, 0.2);
        TestJaccardIndex({10, 15}, {12, 15}, 0.6);
        TestJaccardIndex({10, 15}, {12, 20}, 0.3);
        TestJaccardIndex({10, 15}, {15, 15}, 0.);
        TestJaccardIndex({10, 15}, {15, 20}, 0.);
        TestJaccardIndex({10, 15}, {20, 20}, 0.);
        TestJaccardIndex({10, 15}, {20, 21}, 0.);
    }

    Y_UNIT_TEST(Shift) {
        TInterval interval = {10, 15};
        UNIT_ASSERT_EQUAL(interval, TInterval({10, 15}));
        UNIT_ASSERT_EQUAL(interval += 100, TInterval({110, 115}));
        UNIT_ASSERT_EQUAL(interval, TInterval({110, 115}));
        UNIT_ASSERT_EQUAL(interval -= 50, TInterval({60, 65}));
        UNIT_ASSERT_EQUAL(interval, TInterval({60, 65}));
    }

    Y_UNIT_TEST(Hash) {
        THashMap<TInterval, int> map;
        map[TInterval{1, 5}] = 1;
        map[TInterval{1, 4}] = 2;
        map[TInterval{2, 5}] = 3;
        UNIT_ASSERT_EQUAL(map.at(TInterval{1, 5}), 1);
        UNIT_ASSERT_EQUAL(map.at(TInterval{1, 4}), 2);
        UNIT_ASSERT_EQUAL(map.at(TInterval{2, 5}), 3);
    }

    Y_UNIT_TEST(HashInt) {
        THashMap<TIntInterval, int> map;
        map[TIntInterval{-1, 5}] = 1;
        map[TIntInterval{-1, 4}] = 2;
        map[TIntInterval{-2, 5}] = 3;
        UNIT_ASSERT_EQUAL(map.at(TIntInterval{-1, 5}), 1);
        UNIT_ASSERT_EQUAL(map.at(TIntInterval{-1, 4}), 2);
        UNIT_ASSERT_EQUAL(map.at(TIntInterval{-2, 5}), 3);
    }

    Y_UNIT_TEST(Cast) {
        const TIntInterval a = {1, 3};
        const TInterval b = a.ToInterval<size_t>();
        const TIntInterval c = b.ToInterval<int>();
        UNIT_ASSERT_EQUAL(b, TInterval({1, 3}));
        UNIT_ASSERT_EQUAL(c, TIntInterval({1, 3}));
    }
}
