#include "requests_helper.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

Y_UNIT_TEST_SUITE(RequestsHelperTest) {

Y_UNIT_TEST(PageIdxAndSize) {
    {
        auto [pageIdx, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(0, 9);
        UNIT_ASSERT_EQUAL(pageIdx, 0);
        UNIT_ASSERT_EQUAL(pageSize, 10);
    }
    {
        auto [pageIdx, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(10, 19);
        UNIT_ASSERT_EQUAL(pageIdx, 1);
        UNIT_ASSERT_EQUAL(pageSize, 10);
    }
    {
        auto [pageIdx, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(40, 139);
        UNIT_ASSERT_EQUAL(pageIdx, 0);
        UNIT_ASSERT_EQUAL(pageSize, 140);
    }
    {
        auto [pageIdx, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(100, 119);
        UNIT_ASSERT_EQUAL(pageIdx, 5);
        UNIT_ASSERT_EQUAL(pageSize, 20);
    }

    // corner case - best page size
    {
        auto [pageIdx, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(101, 201);
        UNIT_ASSERT_EQUAL(pageIdx, 1);
        UNIT_ASSERT_EQUAL(pageSize, 101);
    }

    // corner case - worst page size
    {
        auto [pageIdx, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(100, 200);
        UNIT_ASSERT_EQUAL(pageIdx, 0);
        UNIT_ASSERT_EQUAL(pageSize, 201);
    }
}

Y_UNIT_TEST(PageSizeIsNotBig) {
    // check that page size is not bigger than `2 * (r - l) - 1`
    constexpr int LENGTH = 101;
    for (int left = 0; left <= 10000; ++left) {
        int right = left + LENGTH - 1;
        auto [_, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(left, right);
        UNIT_ASSERT(pageSize <= 2 * LENGTH - 1);
    }
}

}

} //namespace NAlice::NHollywood::NMusic
