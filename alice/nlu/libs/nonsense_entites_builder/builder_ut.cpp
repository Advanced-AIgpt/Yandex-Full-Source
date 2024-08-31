#include "builder.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/split.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(TNonsenseEntitiesBuilder) {

    TString FormatResults(const TVector<TNonsenseEntityHypothesis>& results) {
        TStringBuilder out;
        for (const TNonsenseEntityHypothesis& entity : results) {
            out << "    " << entity.Interval << " " << entity.Prob << Endl;
        }
        return out;
    }

    void Test(const TString& tokensStr, const TVector<double>& nonsenseProbs,
        const TVector<TNonsenseEntityHypothesis>& expected)
    {
        const TVector<TString> tokens = StringSplitter(tokensStr).Split(' ').SkipEmpty().ToList<TString>();
        const TVector<TNonsenseEntityHypothesis> actual = TNonsenseEntitiesBuilder(tokens, nonsenseProbs).Build();
        if (expected == actual) {
            return;
        }
        TStringBuilder message;
        message << Endl;
        message << "TNonsenseEntitiesBuilder error:" << Endl;
        message << "  tokens: " << tokensStr << Endl;
        message << "  expected:" << Endl << FormatResults(expected);
        message << "  actual:" << Endl << FormatResults(actual);
        UNIT_FAIL(message);
    }

    Y_UNIT_TEST(TestEmpty) {
        Test("", {}, {});
    }

    Y_UNIT_TEST(TestWhole) {
        Test(
            "алиса",
            {0.993},
            {
                {{0, 1}, 0.993},
            }
        );
    }

    Y_UNIT_TEST(TestZeroProbs) {
        Test(
            "алиса",
            {0.0},
            {}
        );

        Test(
            "алиса привет как дела",
            {0.0, 0.0, 0.0, 0.0},
            {}
        );
    }

    Y_UNIT_TEST(TestTagger) {
        Test(
            "алиса  будь   добра  включи пожалуйста телевизор",
            {0.999, 0.999, 1.000, 0.001, 0.981,     0.001},
            {
                {{0, 3}, 0.999},
                {{4, 5}, 0.981},
            }
        );
        Test(
            "алиса  будь   добра  замолчи",
            {0.999, 0.883, 0.991, 0.449},
            {
                {{0, 3}, 0.883},
                {{0, 4}, 0.449},
            }
        );
        Test(
            "если   ты     не     понимаешь своих  слов   просто отстань от     меня",
            {0.752, 0.805, 0.877, 0.922,    0.996, 0.933, 0.993, 0.246,  0.003, 0.001},
            {
                {{1, 7}, 0.805},
                {{0, 7}, 0.752},
                {{0, 8}, 0.246},
            }
        );
        Test(
            "ты     дура   безмозглая что    ли     включи клип   я      прошу  тебя   успокойся",
            {0.999, 1.000, 0.999,     0.999, 1.000, 0.001, 0.001, 0.415, 0.001, 0.000, 0.001},
            {
                {{0, 5}, 0.999},
                {{7, 8}, 0.415},
            }
        );
        Test(
            "яндекс включи музыку",
            {0.671, 0.002, 0.000},
            {
                {{0, 1}, 0.671},
            }
        );
        Test(
            "яндекс какой  курс   акций  яндекс",
            {0.759, 0.075, 0.000, 0.000, 0.001},
            {
                {{0, 1}, 0.759},
            }
        );
        Test(
            "зачем  ты     включила музыку",
            {0.996, 0.998, 0.006,   0.006},
            {
                {{0, 2}, 0.996},
            }
        );
        Test(
            "зачем  ты     меня   разбудила 6      утра",
            {0.988, 0.998, 0.982, 0.628,    0.911, 0.990},
            {
                {{0, 3}, 0.982},
                {{4, 6}, 0.911},
                {{0, 6}, 0.628},
            }
        );
    }

    Y_UNIT_TEST(TestRepetitions) {
        Test(
            "алиса  алиса  алиса  алиса",
            {0.959, 0.946, 0.979, 0.990},
            {
                {{0, 4}, 0.946},
                {{0, 3}, 0.85},
            }
        );
        Test(
            "сколько сейчас сейчас время",
            {0.062,  0.026, 0.025, 0.005},
            {
                {{1, 2}, 0.85},
            }
        );
        Test(
            "поставь будильник поставь будильник на     7      часов",
            {0.001,  0.000,    0.001,  0.000,    0.000, 0.000, 0.000},
            {
                {{0, 2}, 0.85},
            }
        );
        Test(
            "алиса  давай  еще    алиса  давай  еще",
            {0.998, 0.105, 0.016, 0.952, 0.047, 0.114},
            {
                {{0, 1}, 0.998},
                {{3, 4}, 0.952},
                {{0, 3}, 0.85},
                {{5, 6}, 0.114},
                {{0, 2}, 0.105},
            }
        );
        Test(
            "не     не     надо   не     надо   не     надо",
            {0.259, 0.001, 0.000, 0.000, 0.000, 0.001, 0.000},
            {
                {{0, 5}, 0.85},
                {{0, 1}, 0.259},
            }
        );
    }
}
