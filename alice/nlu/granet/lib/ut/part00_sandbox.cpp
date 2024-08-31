#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart00_Sandbox) {

    // ya make alice/nlu/granet/lib/ut -rt -F 'GranetPart00_Sandbox::HasMatch'
    Y_UNIT_TEST(HasMatch) {
        TGranetTester tester(R"(
            form f:
                root: врубай музон
                filler: алиса
        )");
        // tester.EnableLog(true);
        tester.TestHasMatch("врубай музон алиса", true);
    }

    Y_UNIT_TEST(Tagger) {
        TGranetTester tester(R"(
            form fairy_tale_form:
                slots:
                    fairy_tale:
                        source: $custom.fairy_tale
                        type: custom.fairy_tale
                root:
                    прочитай сказку $custom.fairy_tale
                filler:
                    алиса
                    пожалуйста
        )");
        // tester.EnableLog(true);
        tester.AddEntity("custom.fairy_tale", "колобок", "kolobok", -4);
        tester.TestHasMatch("прочитай сказку колобок пожалуйста", true);
        tester.TestTagger("fairy_tale_form", true, "прочитай сказку 'колобок'(fairy_tale) пожалуйста");
        tester.TestTagger("fairy_tale_form", true, "прочитай сказку 'колобок'(fairy_tale/custom.fairy_tale:kolobok) пожалуйста");
    }
}
