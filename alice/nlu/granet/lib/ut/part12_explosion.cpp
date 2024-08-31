#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart12_Explosion) {

    Y_UNIT_TEST(FillerSequence) {
        TGranetTester tester(R"(
            form f:
                root:   [а* $A+]
                $A:     [б* фу]
                filler: фу
        )");
        tester.TestHasMatch(TString("фу ") * 50, true);
    }

    Y_UNIT_TEST(SearchTillEnd) {
        TGranetTester tester(R"(
            form f:
                root:   [$B+ $A+]
                $A:     .* расскажи .*
                $B:     .* сказку .*
        )");
        tester.TestHasMatch(TString("расскажи ") * 40 + "сказку", true);
    }
}
