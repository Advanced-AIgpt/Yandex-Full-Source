#include <alice/nlu/granet/lib/utils/rle_array.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/buffer.h>
#include <util/ysaveload.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(RleArray) {

    Y_UNIT_TEST(Init) {
        const TRleArray<int> a = {5, 5, 5, 1, 1, 3, 1};
        UNIT_ASSERT_EQUAL(a.size(), 7);
        UNIT_ASSERT_EQUAL(a.ysize(), 7);
        UNIT_ASSERT(!a.empty());
        UNIT_ASSERT_EQUAL(a[0], 5);
        UNIT_ASSERT_EQUAL(a[1], 5);
        UNIT_ASSERT_EQUAL(a[2], 5);
        UNIT_ASSERT_EQUAL(a[3], 1);
        UNIT_ASSERT_EQUAL(a[4], 1);
        UNIT_ASSERT_EQUAL(a[5], 3);
        UNIT_ASSERT_EQUAL(a[6], 1);
        UNIT_ASSERT_EQUAL(a.back(), 1);
        UNIT_ASSERT_EQUAL(a.GetGroupCount(), 4);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(0), 3);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(1), 2);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(2), 1);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(3), 1);
    }

    Y_UNIT_TEST(InitString) {
        const TRleArray<TString> a = {"s5", "s5", "s5", "s1", "s1", "s3", "s1"};
        UNIT_ASSERT_EQUAL(a.size(), 7);
        UNIT_ASSERT_EQUAL(a.ysize(), 7);
        UNIT_ASSERT(!a.empty());
        UNIT_ASSERT_EQUAL(a[0], "s5");
        UNIT_ASSERT_EQUAL(a[1], "s5");
        UNIT_ASSERT_EQUAL(a[2], "s5");
        UNIT_ASSERT_EQUAL(a[3], "s1");
        UNIT_ASSERT_EQUAL(a[4], "s1");
        UNIT_ASSERT_EQUAL(a[5], "s3");
        UNIT_ASSERT_EQUAL(a[6], "s1");
        UNIT_ASSERT_EQUAL(a.back(), "s1");
        UNIT_ASSERT_EQUAL(a.GetGroupCount(), 4);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(0), 3);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(1), 2);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(2), 1);
        UNIT_ASSERT_EQUAL(a.GetGroupLength(3), 1);
    }

    Y_UNIT_TEST(Interators) {
        const TRleArray<int> a = {5, 5, 5, 1, 1, 3, 1};
        const TVector<int> v = {5, 5, 5, 1, 1, 3, 1};
        UNIT_ASSERT_EQUAL(v, TVector<int>(a.begin(), a.end()));
    }

    Y_UNIT_TEST(InteratorsEmpty) {
        const TRleArray<int> a = {};
        const TVector<int> v = {};
        UNIT_ASSERT_EQUAL(v, TVector<int>(a.begin(), a.end()));
    }

    Y_UNIT_TEST(Equal) {
        const TRleArray<int> a1 = {5, 5, 5, 1, 1, 3, 1};
        const TRleArray<int> a2 = {5, 5, 5, 1, 1, 3, 1};
        UNIT_ASSERT(a1 == a2);
        UNIT_ASSERT(!(a1 != a2));
        UNIT_ASSERT_EQUAL(a1, a2);

        const TRleArray<int> a3 = {5, 5, 5, 1, 1, 3, 2};
        UNIT_ASSERT(!(a1 == a3));
        UNIT_ASSERT(a1 != a3);
    }

    Y_UNIT_TEST(Empty) {
        const TRleArray<int> a1;
        UNIT_ASSERT(a1.empty());
        TRleArray<int> a2 = {5, 5, 5, 1, 1, 3, 1};
        UNIT_ASSERT(!a2.empty());
        a2.clear();
        UNIT_ASSERT(a2.empty());
    }

    Y_UNIT_TEST(PushBack) {
        TRleArray<int> a;
        a.push_back(5);
        a.push_back(5);
        a.push_back(5);
        a.push_back(1);
        a.push_back(1);
        a.push_back(3);
        a.push_back(1);
        const TRleArray<int> expected = {5, 5, 5, 1, 1, 3, 1};
        UNIT_ASSERT_EQUAL(a, expected);
    }

    Y_UNIT_TEST(Assign) {
        const TRleArray<int> expected = {5, 5, 5, 1, 1, 3, 1};
        TRleArray<int> a = expected;
        UNIT_ASSERT(a == expected);
        a.clear();
        UNIT_ASSERT(a != expected);
        a = expected;
        UNIT_ASSERT(a == expected);
        a.clear();
        a = {5, 5, 5, 1, 1, 3, 1};
        UNIT_ASSERT(a == expected);
    }

    Y_UNIT_TEST(SerializeInt) {
        const TRleArray<int> a = {5, 5, 5, 1, 1, 3, 1};
        TBufferStream buffer;
        ::Save(&buffer, a);
        TRleArray<int> b;
        ::Load(&buffer, b);
        UNIT_ASSERT_EQUAL(a, b);
    }

    Y_UNIT_TEST(SerializeString) {
        const TRleArray<TString> a = {"s5", "s5", "s5", "s1", "s1", "s3", "s1"};
        TBufferStream buffer;
        ::Save(&buffer, a);
        TRleArray<TString> b;
        ::Load(&buffer, b);
        UNIT_ASSERT_EQUAL(a, b);
    }
}
