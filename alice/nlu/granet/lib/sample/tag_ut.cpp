#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(TTag) {

    bool TestPadding(const TVector<TTag>& original, size_t length, const TVector<TTag>& expected) {
        return NGranet::AddPadding(original, length) == expected;
    }

    Y_UNIT_TEST(AddPadding) {
        UNIT_ASSERT(TestPadding({}, 0, {}));
        UNIT_ASSERT(TestPadding({}, 5, {MakeTag(0, 5)}));
        UNIT_ASSERT(TestPadding(
            {               MakeTag(2, 2, "B")            }, 5,
            {MakeTag(0, 2), MakeTag(2, 2, "B"), MakeTag(2, 5)}));
        UNIT_ASSERT(TestPadding(
            {MakeTag(0, 1, "B")               }, 5,
            {MakeTag(0, 1, "B"), MakeTag(1, 5)}));
        UNIT_ASSERT(TestPadding(
            {MakeTag(0, 1, "B"),                MakeTag(2, 3, "A")}, 3,
            {MakeTag(0, 1, "B"), MakeTag(1, 2), MakeTag(2, 3, "A")}));
        UNIT_ASSERT(TestPadding(
            {MakeTag(0, 2, "B"), MakeTag(2, 3, "A")}, 3,
            {MakeTag(0, 2, "B"), MakeTag(2, 3, "A")}));
        UNIT_ASSERT(TestPadding(
            {MakeTag(0, 2, "B"), MakeTag(1, 3, "A")}, 3,
            {MakeTag(0, 2, "B"), MakeTag(1, 3, "A")}));
        UNIT_ASSERT(TestPadding(
            {MakeTag(2, 3, "A"), MakeTag(0, 2, "B")}, 3,
            {MakeTag(0, 2, "B"), MakeTag(2, 3, "A")}));
        UNIT_ASSERT(TestPadding(
            {               MakeTag(2, 4, "A"),                MakeTag(6, 8, "B"),                 MakeTag(10, 12, "C")}, 8,
            {MakeTag(0, 2), MakeTag(2, 4, "A"), MakeTag(4, 6), MakeTag(6, 8, "B"), MakeTag(8, 10), MakeTag(10, 12, "C")}));
        UNIT_ASSERT(TestPadding(
            {               MakeTag(2, 4, "A"),                MakeTag(6, 8, "B"),                 MakeTag(10, 12, "C")}, 9,
            {MakeTag(0, 2), MakeTag(2, 4, "A"), MakeTag(4, 6), MakeTag(6, 8, "B"), MakeTag(8, 10), MakeTag(10, 12, "C")}));
        UNIT_ASSERT(TestPadding(
            {               MakeTag(2, 4, "A"),                MakeTag(6, 8, "B"),                 MakeTag(10, 12, "C")}, 10,
            {MakeTag(0, 2), MakeTag(2, 4, "A"), MakeTag(4, 6), MakeTag(6, 8, "B"), MakeTag(8, 10), MakeTag(10, 12, "C")}));
        UNIT_ASSERT(TestPadding(
            {               MakeTag(2, 4, "A"),                MakeTag(6, 8, "B"),                 MakeTag(10, 12, "C")}, 15,
            {MakeTag(0, 2), MakeTag(2, 4, "A"), MakeTag(4, 6), MakeTag(6, 8, "B"), MakeTag(8, 10), MakeTag(10, 12, "C"), MakeTag(12, 15)}));
    }

    void TestMarkup(TStringBuf text, TStringBuf markup, const TVector<TTag>& tags) {
        NAlice::NUtUtils::TestEqualStr(text, markup, PrintTaggerMarkup(text, tags));
        NAlice::NUtUtils::TestEqualStr(markup, text, RemoveTaggerMarkup(markup));

        TString actualText;
        TVector<TTag> actualTags;
        const bool ok = TryReadTaggerMarkup(markup, &actualText, &actualTags);
        UNIT_ASSERT(ok);
        NAlice::NUtUtils::TestEqualSeq(markup, tags, actualTags);
        NAlice::NUtUtils::TestEqualStr(markup, text, actualText);
    }

    Y_UNIT_TEST(Markup) {
        TestMarkup(
            "a b c",
            "'a'(t1) b 'c'(t2)",
            {MakeTag(0, 1, "t1"), MakeTag(4, 5, "t2")});
        TestMarkup(
            "abc",
            "'a'b'(t1)c'(t2)",
            {MakeTag(0, 3, "t2"), MakeTag(1, 2, "t1")});
        TestMarkup(
            "abc",
            "'a'(t1)'bc'(t2)",
            {MakeTag(0, 1, "t1"), MakeTag(1, 3, "t2")});
        TestMarkup(
            "abc",
            "a''(t1)bc",
            {MakeTag(1, 1, "t1")});
        TestMarkup(
            "abc",
            "a'''(t1)'(t2)bc",
            {MakeTag(1, 1, "t1"), MakeTag(1, 1, "t2")});
        TestMarkup(
            "abc",
            "'a'(t1)'''(t2)'(t3)'bc'(t4)",
            {MakeTag(0, 1, "t1"), MakeTag(1, 1, "t2"), MakeTag(1, 1, "t3"), MakeTag(1, 3, "t4")});
        TestMarkup(
            "abc",
            "''a'(t1)''b'(t2)'c'(t3)'(t4)'(t5)",
            {MakeTag(0, 1, "t1"), MakeTag(0, 3, "t5"), MakeTag(1, 2, "t2"), MakeTag(1, 3, "t4"), MakeTag(2, 3, "t3")});
        TestMarkup(
            "abc",
            "''a'(t5)''b'(t4)'c'(t3)'(t2)'(t1)",
            {MakeTag(0, 1, "t5"), MakeTag(0, 3, "t1"), MakeTag(1, 2, "t4"), MakeTag(1, 3, "t2"), MakeTag(2, 3, "t3")});
        // Issue DIALOG-6912
        TestMarkup(
            "01 06ю2005",
            "'01'(timer_id) '06ю'(timer_id)'2005'(timer_id)",
            {MakeTag(0, 2, "timer_id"), MakeTag(3, 7, "timer_id"), MakeTag(7, 11, "timer_id")});
        TestMarkup(
            "Раз два три привет",
            "'Раз два'(tag 1) три 'привет'(tag 2)",
            {MakeTag(0, 13, "tag 1"), MakeTag(21, 33, "tag 2")});
        TestMarkup(
            "Пример вложенных тегов раз два три",
            R"(Пример 'вложенных 'тегов'(tag) раз'(tag:" 'with \" (special) symbols") два три)",
            {MakeTag(13, 49, R"(tag:" 'with \" (special) symbols")"), MakeTag(32, 42, "tag")});
    }
}
