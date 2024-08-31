#include "alignment.h"
#include "joined_tokens.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>

using namespace NNlu;

Y_UNIT_TEST_SUITE(TAlignmentData) {

    Y_UNIT_TEST(Empty) {
        TAlignmentData empty;
        UNIT_ASSERT(empty.IsEmpty());
        UNIT_ASSERT(empty.IsValid());
    }

    void TestBasic(size_t tokenCount1, size_t tokenCount2, TStringBuf str, const TAlignmentData& alignment) {
        UNIT_ASSERT_VALUES_EQUAL(tokenCount1, alignment.CountTokens1());
        UNIT_ASSERT_VALUES_EQUAL(tokenCount2, alignment.CountTokens2());
        UNIT_ASSERT_STRINGS_EQUAL(str, alignment.WriteToString());
        UNIT_ASSERT_VALUES_EQUAL(alignment, TAlignmentData::ReadFromString(str));
    }

    Y_UNIT_TEST(Basic) {
        TestBasic(0, 0, "", {});
        TestBasic(1, 1, "=", {{1}, {1}, {true}});
        TestBasic(1, 1, "~", {{1}, {1}, {false}});
        TestBasic(2, 3, "2=3", {{2}, {3}, {true}});
        TestBasic(5, 7, "2=3/=/=/1~2", {{2, 1, 1, 1}, {3, 1, 1, 2}, {true, true, true, false}});
    }

    void TestAlignment(const TAlignmentData& actual, const TAlignmentData& expected) {
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    void TestAlignment(const TAlignmentData& actual, TStringBuf expected) {
        UNIT_ASSERT_VALUES_EQUAL(actual, TAlignmentData::ReadFromString(expected));
    }

    Y_UNIT_TEST(MergeTokens) {
        TAlignmentData alignment = TAlignmentData::ReadFromString("=/3=2/4~3");

        alignment.MergeTokens1({1, 1, 1, 1, 1, 1, 1, 1});
        TestAlignment(alignment, "=/3=2/4~3");

        alignment.MergeTokens2({1, 1, 1, 1, 1, 1});
        TestAlignment(alignment, "=/3=2/4~3");

        alignment.MergeTokens1({1, 2, 1, 1, 1, 1, 1});
        TestAlignment(alignment, "=/2=2/4~3");

        alignment.MergeTokens2({1, 1, 3, 1});
        TestAlignment(alignment, "=/6~3");

        alignment.MergeTokens1({7});
        TestAlignment(alignment, "1~4");

        alignment.MergeTokens2({1, 2, 1});
        TestAlignment(alignment, "1~3");

        alignment.MergeTokens2({3});
        TestAlignment(alignment, "~");
    }

}
