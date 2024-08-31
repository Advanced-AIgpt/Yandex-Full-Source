#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart10_Compound) {

    Y_UNIT_TEST(OrInsideOr) {
        TGranetTester tester(R"(
            form f:
                root: a | b c | (d | e) | (f)
        )");

        tester.TestHasMatch({
            {"a", true},
            {"b c", true},
            {"d", true},
            {"e", true},
            {"f", true},

            {"", false},
            {"a b c", false},
            {"d e", false}
        });
    }

    Y_UNIT_TEST(Question) {
        TGranetTester tester(R"(
            form f:
                root:
                    a
                    b c? (d e)? f
                    g h
        )");

        tester.TestHasMatch({
            {"a", true},
            {"b f", true},
            {"b c f", true},
            {"b c d e f", true},
            {"b d e f", true},
            {"g h", true},

            {"b c d f", false},
            {"b e f", false}
        });
    }

    Y_UNIT_TEST(Bag) {
        TGranetTester tester(R"(
            form f:
                root: a | B c | (d | e) | (f) | [g h* i? j+] | k [l m]
        )");

        tester.TestHasMatch({
            {"a", true},
            {"b C", true},
            {"d", true},
            {"e", true},
            {"f", true},
            {"g j", true},
            {"h j h j g i h", true},
            {"k l m", true},
            {"k m l", true},

            {"g", false},
            {"h j", false},
            {"k", false}
        });
    }

    Y_UNIT_TEST(TopLevelBag) {
        TGranetTester tester(R"(
            form f:
                root: [(a b) (c | d) (e f)* (g h)? (i | j k)+]
        )");

        tester.TestHasMatch({
            {"a b c i", true},
            {"a b c i", true},
            {"g h j k c e f e f e f a b e f e f e f i", true},

            {"a c b i", false},
            {"c i a b e i f", false}
        });
    }
}
