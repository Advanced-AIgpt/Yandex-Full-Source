#include <alice/boltalka/libs/text_utils/utterance_transform.h>
#include <alice/boltalka/libs/text_utils/context_transform.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NNlgTextUtils;

namespace {

static const TString TestString = " Эт0 -- VeRy haRd  тЁсТ, которыЙ должËн провёритb   vsë~! ЁËёë.DELETE";
static const TString AnswerString = "эт0 M M very hard тест L который должен проверитb vsе F0 A ееее N";

}

Y_UNIT_TEST_SUITE(TNlgTextUtilsTest) {

    Y_UNIT_TEST(TestUtteranceTransform) {
        const IUtteranceTransformPtr utteranceTransform = new TCompoundUtteranceTransform({
            new TLowerCase(ELanguage::LANG_RUS),
            new TSeparatePunctuation,
            new TLimitNumTokens(15),
            new TMapYo,
            new TReplacePunct4Insight
        });
        UNIT_ASSERT_EQUAL(utteranceTransform->Transform(TestString), AnswerString);
    }

    Y_UNIT_TEST(TestUtteranceWiseTransform) {
        const IUtteranceTransformPtr utteranceTransform = new TCompoundUtteranceTransform({
            new TLowerCase(ELanguage::LANG_RUS),
            new TSeparatePunctuation,
            new TLimitNumTokens(15),
            new TMapYo,
            new TReplacePunct4Insight
        });
        TUtteranceWiseTransform utteranceWise(utteranceTransform);
        TVector<TString> test(10, TestString);
        TVector<TString> answer(10, AnswerString);
        UNIT_ASSERT_EQUAL(utteranceWise.Transform(test), answer);
    }

    Y_UNIT_TEST(TestSetContextNumTurns) {
        TSetContextNumTurns more(10);
        TVector<TString> test(5, TestString);
        TVector<TString> answer(5, TestString);
        for (size_t i = 0; i < 5; ++i) {
            answer.push_back("");
        }
        UNIT_ASSERT_EQUAL(more.Transform(test), answer);

        TSetContextNumTurns less(3);
        answer.resize(3);
        UNIT_ASSERT_EQUAL(less.Transform(test), answer);

        TCompoundContextTransform both({
            new TSetContextNumTurns(10),
            new TSetContextNumTurns(3)
        });
        UNIT_ASSERT_EQUAL(both.Transform(answer), answer);
    }

    Y_UNIT_TEST(TestCutAlice) {
        const TVector<TString> test = {
            "нет , я - алиса",
            "я - алиса",
            "алис алис"
        };
        const TVector<TString> answer = {
            "нет , я -",
            "я - алиса",
            "алис"
        };
        TCutAliceFromUser cut;
        UNIT_ASSERT_EQUAL(cut.Transform(test), answer);
    }
}

