#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart03_Wildcard) {

    // ~~~~ Wildcard ~~~~

    Y_UNIT_TEST(SingleWildcardOnly) {
         TGranetTester tester(R"(
            form f:
                root: .
        )");

        tester.TestHasMatch({
            {"1", true},
            {"x", true},

            {"", false},
            {"1 2", false}
        });
    }

    Y_UNIT_TEST(SingleWildcard) {
        TGranetTester tester(R"(
            form f:
                root: . x
        )");

        tester.TestHasMatch({
            {"1 x", true},
            {"x x", true},

            {"", false},
            {"x", false},
            {"1 2 x", false},
            {"x x x", false}
        });
    }

    Y_UNIT_TEST(SingleWildcardDouble) {
        TGranetTester tester(R"(
            form f:
                root: . .
        )");

        tester.TestHasMatch({
            {"1 2", true},
            {"x x", true},

            {"", false},
            {"x", false},
            {"x x x", false}
        });
    }

    Y_UNIT_TEST(SingleWildcardMany) {
        TGranetTester tester(R"(
            form f:
                root: . x . y . z .
        )");

        tester.TestHasMatch({
            {"1 x 2 y 3 z 4", true},
            {"x x x y z z z", true},

            {"", false},
            {"x y z", false},
            {"1 x 2 y 3 z", false},
            {"x 2 y 3 z 4", false},
            {"1 x y 3 z 4", false}
        });
    }

    Y_UNIT_TEST(MultiWildcardOnly) {
        TGranetTester tester(R"(
            form f:
                root: .*
        )");

        tester.TestHasMatch({
            {"", true},
            {"1", true},
            {"1 1 1 1", true},
            {"1 2 3 4 5", true}
        });
    }

    Y_UNIT_TEST(MultiWildcardInside) {
        TGranetTester tester(R"(
            form f:
                root: x .* y
        )");

        tester.TestHasMatch({
            {"x y", true},
            {"x 1 y", true},
            {"x y y", true},
            {"x x y", true},
            {"x 1 1 1 1 y", true},
            {"x 1 2 3 4 5 y", true},
            {"x x x x y y y", true},

            {"", false},
            {"x", false},
            {"y", false},
            {"x y 1", false},
            {"1 x y", false}
        });
    }

    Y_UNIT_TEST(MultiWildcardLeft) {
        TGranetTester tester(R"(
            form f:
                root: .* x
        )");

        tester.TestHasMatch({
            {"x", true},
            {"1 2 3 x", true},
            {"x x x x", true},

            {"", false},
            {"1", false},
            {"x 1", false},
            {"x x 1", false}
        });
    }

    Y_UNIT_TEST(MultiWildcardRight) {
        TGranetTester tester(R"(
            form f:
                root: x .*
        )");

        tester.TestHasMatch({
            {"x", true},
            {"x 1 2 3", true},
            {"x x x x", true},

            {"", false},
            {"1", false},
            {"1 x", false},
            {"1 x x", false}
        });
    }

    Y_UNIT_TEST(MultiWildcardContacted) {
        TGranetTester tester(R"(
            form f:
                root: x .* .* y
        )");

        tester.TestHasMatch({
            {"x y", true},
            {"x 1 y", true},
            {"x y y", true},
            {"x y y y", true},
            {"x y y y y", true},
            {"x 1 2 3 4 5 y", true},
            {"x 1 2 y 4 5 y", true},

            {"", false},
            {"x", false},
            {"y", false},
            {"1", false},
            {"1 x y", false},
            {"x y 1", false},
            {"x y y y y 1", false}
        });
    }

    Y_UNIT_TEST(MultiWildcardSearch) {
        TGranetTester tester(R"(
            form f:
                root:
                    .* x y z .*
                    .* x y a .*
                    .* a b .*
                    r s .*
        )");

        tester.TestHasMatch({
            {"x y z", true},
            {"x y a", true},
            {"a b", true},
            {"r s", true},

            {"1 1 x y z 1 1", true},
            {"1 1 x y a 1 1", true},
            {"1 1 a b 1 1", true},
            {"r s 1 1", true},

            {"x 1 1 1 x y a", true},
            {"x 1 1 x y z 1", true},
            {"r x y a 1", true},
            {"r s x y a 1", true},
            {"1 a b 1", true},
            {"x y 1 1 1 x y z", true},
            {"1 1 r 1 x y 1 x 1 1 x y z 1 1 1", true},
            {"x y z 1 1 1 x y 1 1 1 z 1 1 1", true},
            {"x x x y x y x z x x y z x", true},

            {"", false},
            {"x y", false},
            {"x y b", false},
            {"1 r s", false},
            {"x x y r s x y b 1", false},
            {"x y 1 z", false},
            {"1 1 x y 1 z 1 1", false}
        });
    }

    Y_UNIT_TEST(WildcardPlus) {
        TGranetTester tester(R"(
            form f:
                root: .+ x .+
        )");

        tester.TestHasMatch({
            {"1 x 1", true},
            {"1 1 x 1 1", true},
            {"1 x x", true},
            {"x x 1", true},
            {"x x x", true},
            {"1 1 x 1 1 x 1 1 x 1 1", true},
            {"x 1 1 x 1 1 x 1 1", true},
            {"1 1 x 1 1 x 1 1 x", true},

            {"", false},
            {"x", false},
            {"1 x", false},
            {"x 1", false},
            {"x 1 x", false}
        });
    }

    Y_UNIT_TEST(WildcardQuantity) {
        TGranetTester tester(R"(
            form f:
                root: .<2> x .<3,5>
        )");

        tester.TestHasMatch({
            {"1 2 x 1 2 3", true},
            {"1 2 x 1 2 3 5", true},
            {"1 2 x 1 2 3 4 5", true},

            {"x 1 2 3", false},
            {"1 x 1 2 3", false},
            {"1 2 3 x 1 2 3", false},
            {"1 2 x", false},
            {"1 2 x 1 2", false},
            {"1 2 x 1 2 3 4 5 6", false},
        });
    }

    Y_UNIT_TEST(WildcardQuantityDefault) {
        TGranetTester tester(R"(
            form f:
                root: .<2,> x .<,2>
        )");

        tester.TestHasMatch({
            {"1 2 x", true},
            {"1 2 3 x", true},
            {"1 2 3 4 5 6 7 8 9 x", true},
            {"1 2 x 1", true},
            {"1 2 x 1 2", true},

            {"x", false},
            {"1 x", false},
            {"1 2 x 1 2 3", false},
        });
    }

    Y_UNIT_TEST(WildcardInTree) {
        TGranetTester tester(R"(
            form f:
                root:  $B x $C y $D
                $B: .*
                $C: .+
                $D: .* | 1 2
        )");

        tester.TestHasMatch({
            {"x 1 y", true},
            {"x x x 1 y", true},
            {"x x x 1 y 1", true},
            {"x 1 y 1", true},
            {"x 1 y 1 2", true},
            {"x 1 y 1 2 3", true},
            {"x x y", true},
            {"x y x x y", true},

            {"", false},
            {"x", false},
            {"y", false},
            {"x y", false},
            {"x y x 1 2", false}
        });
    }
}
