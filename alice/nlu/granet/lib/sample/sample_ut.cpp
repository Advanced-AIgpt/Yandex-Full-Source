#include "sample.h"
#include <library/cpp/testing/unittest/registar.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(TSample) {

    Y_UNIT_TEST(Test) {
        TSample::TConstRef sample1 = TSample::Create("  Заправь-ка   АИ95 на $8.5. Всё?", LANG_RUS);
        UNIT_ASSERT((sample1->GetText() == "  Заправь-ка   АИ95 на $8.5. Всё?"));
        UNIT_ASSERT((sample1->GetTokens() == TVector<TString>{"заправь", "ка", "аи", "95", "на", "8", "5", "все"}));
        UNIT_ASSERT((sample1->GetTokensIntervals() ==
            TVector<NNlu::TInterval>{{2, 16}, {17, 21}, {24, 28}, {28, 30}, {31, 35}, {37, 38}, {39, 40}, {42, 48}}));

        TSample::TConstRef sample2 = TSample::Create(
            sample1->GetText(),
            sample1->GetLanguage(),
            sample1->GetTokens(),
            sample1->GetTokensIntervals());
        UNIT_ASSERT(sample2->GetText() == sample1->GetText());
        UNIT_ASSERT(sample2->GetTokens() == sample1->GetTokens());
        UNIT_ASSERT(sample2->GetTokensIntervals() == sample1->GetTokensIntervals());

        TSample::TConstRef sample3 = TSample::CreateFromTokens(sample1->GetTokens(), LANG_RUS);
        UNIT_ASSERT(sample3->GetText() == "заправь ка аи 95 на 8 5 все");
        UNIT_ASSERT(sample3->GetTokens() == sample1->GetTokens());
        UNIT_ASSERT((sample3->GetTokensIntervals() ==
            TVector<NNlu::TInterval>{{0, 14}, {15, 19}, {20, 24}, {25, 27}, {28, 32}, {33, 34}, {35, 36}, {37, 43}}));

    }
}
