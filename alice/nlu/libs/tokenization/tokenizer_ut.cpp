#include "tokenizer.h"

#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

#include <utility>

using namespace NNlu;

namespace {
    template<typename TTokenizerClass, bool normalized = true>
    void TestTokens(ELanguage lang, const TVector<std::pair<TStringBuf, TStringBuf>>& data) {
        for (const auto& [text, expected] : data) {
            auto tokenizer = TTokenizerClass(text, lang);
            TVector<TString> tokens;

            if (normalized) {
                tokens = tokenizer.GetNormalizedTokens();
            } else {
                tokens = tokenizer.GetOriginalTokens();
            }

            const TString actual = JoinSeq(" ", tokens);
            NAlice::NUtUtils::TestEqual(text, expected, actual);
        }
    }
} // namespace

Y_UNIT_TEST_SUITE(TTokenizer) {
    void TestIntervals(ELanguage lang, const TVector<std::pair<TStringBuf, TVector<NNlu::TInterval>>>& data) {
        for (const auto& [text, expected] : data) {
            const TVector<NNlu::TInterval> actual = TSmartTokenizer(text, lang).GetTokensIntervals();
            NAlice::NUtUtils::TestEqualSeq(text, expected, actual);
        }
    }

    Y_UNIT_TEST(Tokens) {
        TestTokens<TSmartTokenizer>(LANG_RUS, {
            {"  d.-+ББ  \t ВВ   ",    "d бб вв"},
            {"АИ95",                  "аи 95"},
            {"Включи-ка",             "включи ка"},
            {"don't",                 "don t"},
            {"8.5",                   "8 5"},
            {"$8",                    "8"},
            {"a$b",                   "a b"},
            {"a@b",                   "a b"},
            {"всё",                   "все"},
            {"fr%ed%eric chopin",     "fr ed eric chopin"},
            // FIXME(samoylovboris)
            // {"догово́р",               "договор"},
        });
    }

    Y_UNIT_TEST(Intervals) {
        TestIntervals(LANG_RUS, {
            {"  d.-+ББ  \t ВВ   ", {{2, 3}, {6, 10}, {14, 18}}},
            {"АИ95",               {{0, 4}, {4, 6}}},
        });
    }

    Y_UNIT_TEST(TokensTurkish) {
        TestTokens<TSmartTokenizer>(LANG_TUR, {
            {"statik hız kamerası", "statik hız kamerası"},
            {"STATİK HIZ KAMERASI", "statik hız kamerası"},
        });
    }
}

Y_UNIT_TEST_SUITE(TWhiteSpaceTokenizer) {
    Y_UNIT_TEST(TokensNormalized) {
        TestTokens<TWhiteSpaceTokenizer>(LANG_RUS, {
            {"  d.-+ББ  \t ВВ   ",    "d.-+бб вв"},
            {"АИ95",                  "аи95"},
            {"Включи-ка",             "включи-ка"},
            {"don't",                 "don't"},
            {"8.5",                   "8.5"},
            {"$8",                    "$8"},
            {"a$b",                   "a$b"},
            {"a@b",                   "a@b"},
            {"всё",                   "все"},
            {"fr%ed%eric chopin",     "fr%ed%eric chopin"},
        });
    }

    Y_UNIT_TEST(Tokens) {
        TestTokens<TWhiteSpaceTokenizer, false>(LANG_RUS, {
            {"  d.-+ББ  \t ВВ   ",    "d.-+ББ ВВ"},
            {"АИ95",                  "АИ95"},
            {"Включи-ка",             "Включи-ка"},
            {"don't",                 "don't"},
            {"8.5",                   "8.5"},
            {"$8",                    "$8"},
            {"a$b",                   "a$b"},
            {"a@b",                   "a@b"},
            {"всё",                   "всё"},
            {"fr%ed%eric chopin",     "fr%ed%eric chopin"},
        });
    }

    Y_UNIT_TEST(TokensTurkishNormalized) {
        TestTokens<TWhiteSpaceTokenizer>(LANG_TUR, {
            {"statik hız kamerası", "statik hız kamerası"},
            {"STATİK HIZ KAMERASI", "statik hız kamerası"},
        });
    }

    Y_UNIT_TEST(TokensTurkish) {
        TestTokens<TWhiteSpaceTokenizer, false>(LANG_TUR, {
            {"statik hız kamerası", "statik hız kamerası"},
            {"STATİK HIZ KAMERASI", "STATİK HIZ KAMERASI"},
        });
    }
}
