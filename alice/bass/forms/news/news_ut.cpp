#include "news.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NBASS::NNews {

namespace {

struct TTestCase {
    TStringBuf title;
    TStringBuf text;
    float similarity;
};

} // namespace

Y_UNIT_TEST_SUITE(TNewsUnitTest) {
    Y_UNIT_TEST(TestTextComparer) {
        TTextComparer comparer;

        // Lemmas: apple снять пятичасовой фильм эрмитаж.
        const TStringBuf appleTitle = "Apple сняла пятичасовой фильм об Эрмитаже";
        TTestCase test_cases[] = {
            {appleTitle, "Apple сняла пятичасовой фильм об Эрмитаже", 1.0},
            {appleTitle, "Новости. Apple сняла пятичасовой фильм об Эрмитаже", 0.0},
            {appleTitle, "Известная компания опубликовала ролик о главном музее Северной столицы.", 0.0},
            {appleTitle,
             "Компания Apple сняла пятичасовой фильм на устройство своего производства, не делая перерывов и не "
             "«склеивая» видео",
             0.8},
            {appleTitle, "Компания Apple поделилась фильмом-путешествием по Эрмитажу.", 0.6},
            {appleTitle,
             "Совместный пятичасовой фильм сняли одним кадром и на одном заряде смартфона iPhone 11 Pro Max.", 0.6},
            // Erasing words lt 3 chars, expect no zero divide exception.
            {"пл ох ой t i t l e", "Хороший сниппет", 0.0},
            {"", "Сниппет", 0.0},
            {"Заголовок", "", 0.0}};
        for (const auto test : test_cases) {
            const float similarity = comparer.GetSimilarity(test.title, comparer.GetFirstSentence(test.text));
            UNIT_ASSERT_VALUES_EQUAL(test.similarity, similarity);
        }
    }
}

} // namespace NBASS
