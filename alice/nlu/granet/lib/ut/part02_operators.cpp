#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart02_Operators) {

    // ~~~~ Quantity operators with element ~~~~

    Y_UNIT_TEST(OperatorStar) {
        TGranetTester tester(R"(
            form f:
                root: $B*
                $B: b
        )");

        tester.TestHasMatch({
            {"", true},
            {"b", true},
            {"b b b b b b", true},

            {"a", false},
            {"b a", false}
        });
    }

    Y_UNIT_TEST(OperatorPlus) {
        TGranetTester tester(R"(
            form f:
                root: $B+
                $B: b
        )");

        tester.TestHasMatch({
            {"b", true},
            {"b b b b b b", true},

            {"", false},
            {"a", false},
            {"b a", false}
        });
    }

    Y_UNIT_TEST(OperatorQuestion) {
        TGranetTester tester(R"(
            form f:
                root: $B?
                $B: b
        )");

        tester.TestHasMatch({
            {"", true},
            {"b", true},

            {"b b b b b b", false},
            {"a", false},
            {"b a", false}
        });
    }

    Y_UNIT_TEST(OperatorStarWithOther) {
        TGranetTester tester(R"(
            form f:
                root: $B* a
                $B: b
        )");

        tester.TestHasMatch({
            {"a", true},
            {"b a", true},
            {"b b b b b b a", true},

            {"b", false},
            {"a b", false}
        });
    }

    Y_UNIT_TEST(OperatorStarAmbiguous) {
        TGranetTester tester(R"(
            form f:
                root: $B* $B*
                $B: b
        )");

        tester.TestHasMatch({
            {"", true},
            {"b", true},
            {"b b b b b b", true},

            {"a", false},
            {"b a", false}
        });
    }

    Y_UNIT_TEST(OperatorStarVariants) {
        TGranetTester tester(R"(
            form f:
                root: $B*
                $B: 1 | 1 5 6 | 2 | 6 7
        )");

        tester.TestHasMatch({
            {"", true},
            {"1", true},
            {"2 1 5 6", true},
            {"1 6 7 1 2", true},

            {"2 1 5 1", false},
            {"2 5", false},
            {"1 5 5", false},
            {"1 5 6 7", false}
        });
    }

    Y_UNIT_TEST(OperatorRange) {
        TGranetTester tester(R"(
            form f:
                root: $B<1,3> $C<2,>
                $B: 1 2 | a b
                $C: x y | z
        )");

        tester.TestHasMatch({
            {"1 2 z z", true}, // B: 1, C: 2
            {"1 2 1 2 z z z", true}, // B: 2, C: 3
            {"1 2 a b 1 2 z z x y z z z", true}, // B: 3, C: 6

            {"1 2", false}, // B: 1, C: 0
            {"1 2 z", false}, // B: 1, C: 1
            {"z z", false}, // B: 0, C: 2
            {"1 2 1 2 1 2 1 2 z z", false}, // B: 4, C: 2
        });
    }

    Y_UNIT_TEST(FindSplitPosition) {
        TGranetTester tester(R"(
            form f:
                root: $B* $D $C*
                $B: 1 | 2 | 3 | uuu
                $C: a | b | c | uuu
                $D: xxx | yyy | uuu
        )");

        tester.TestHasMatch({
            {"1 2 2 xxx c b", true},
            {"1 2 2 yyy c b", true},
            {"1 2 2 uuu c b", true},
            {"1 2 2 xxx uuu c b", true},
            {"1 uuu 2 2 uuu c b", true},
            {"xxx a", true},
            {"xxx", true},
            {"uuu uuu xxx", true},

            {"1 xxx 2 a b c", false},
            {"1 xxx 2 a uuu b c", false},
            {"1 uuu 2 a uuu b c", false},
            {"1 xxx xxx a b", false}
        });
    }

    // ~~~~ Quantity operators with words ~~~~

    Y_UNIT_TEST(WordQuantity) {
        TGranetTester tester(R"(
            form f:
                root: a b? c+ d* e<2> f<,3> g<1,3> h<2,> i
        )");

        tester.TestHasMatch({
            {"a   c               e e       g     h h         i", true},
            {"a b c c c c d d d d e e f f f g g g h h h h h h i", true},

            // Lower bound
            // a    b?   c+  d*  e{2}   f{,3}    g{1,3}   h{2,}  i
            {"a          c       e e             g        h h    i", true},
            {"           c       e e             g        h h    i", false},
            {"a                  e e             g        h h    i", false},
            {"a          c         e             g        h h    i", false},
            {"a          c       e e                      h h    i", false},
            {"a          c       e e             g          h    i", false},
            {"a          c       e e             g        h h     ", false},

            // Upper bound
            // a    b?   c+  d*  e{2}   f{,3}    g{1,3}   h{2,}  i
            {" a    b    c       e e    f f f    g g g    h h    i  ", true},
            {" a a  b    c       e e    f f f    g g g    h h    i  ", false},
            {" a    b b  c       e e    f f f    g g g    h h    i  ", false},
            {" a    b    c       e e e  f f f    g g g    h h    i  ", false},
            {" a    b    c       e e    f f f f  g g g    h h    i  ", false},
            {" a    b    c       e e    f f f    g g g g  h h    i  ", false},
            {" a    b    c       e e    f f f    g g g    h h    i i", false},
        });
    }

    Y_UNIT_TEST(TokenizedWordQuantity) {
        TGranetTester tester(R"(
            form f:
                root: АИ95? всё-таки*
        )");

        tester.TestHasMatch({
            {"", true},
            {"АИ95", true},
            {"АИ 95", true},
            {"АИ", false},
            {"АИ95 всё таки всё таки всё таки", true},
            {"АИ95 всё таки всё таки всё таки всё", false}
        });
    }
}
