#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart06_Bag) {

    Y_UNIT_TEST(SimpleBag) {
        TGranetTester tester(R"(
            form f:
                root: [a b c]
        )");

        tester.TestHasMatch({
            {"a b c", true},
            {"a c b", true},
            {"b a c", true},
            {"b c a", true},
            {"c a b", true},
            {"c b a", true},

            {"", false},
            {"a", false},
            {"x", false},
            {"a", false},
            {"a b", false},
            {"a b c c", false}
        });
    }

    Y_UNIT_TEST(SimpleRealBag) {
        TGranetTester tester(R"(
            form f:
                root: [мама мыла раму]
        )");

        tester.TestHasMatch({
            {"мама мыла раму", true},
            {"мама раму мыла", true},
            {"мыла мама раму", true},
            {"мыла раму мама", true},
            {"раму мыла мама", true},
            {"раму мама мыла", true},

            {"", false},
            {"мама", false},
            {"мама мыла", false},
            {"мама мыла мыла раму", false},
        });
    }

    Y_UNIT_TEST(BagWithOperators) {
        TGranetTester tester(R"(
            form f:
                root: [d+ c* b? a]
        )");

        tester.TestHasMatch({
            {"d c b a", true},
            {"a b c d", true},
            {"a d", true},
            {"a b c c c d d d d d", true},

            {"", false},
            {"x", false},
            {"a", false},
            {"d", false}
        });
    }

    Y_UNIT_TEST(BagWithFillers) {
        TGranetTester tester(R"(
            form f:
                root: [dd+ cc* bb? aa]
                filler: x
        )");

        tester.TestHasMatch({
            {"dd cc bb aa", true},
            {"aa bb cc dd", true},
            {"aa dd", true},
            {"aa bb cc cc cc dd dd dd dd dd", true},

            {"x x dd x x cc x x bb x x aa x x", true},
            {"x x aa x x bb x x cc x x dd x x", true},
            {"x x aa x x dd x x", true},
            {"x x aa x x bb x x cc x x cc x x cc x x dd x x dd x x dd x x dd x x dd x x", true},

            {"", false},
            {"x", false},
            {"aa", false},
            {"x x aa x x", false},
            {"dd", false}
        });
    }

    Y_UNIT_TEST(CollapseOfDuplicatedElementsInBag) {
        TGranetTester tester(R"(
            form f:
                root:
                    [a1 a1]         # works due to grammar optimizer: [a b] = (a b | b a)
                    [a2 a2?]        # doesn't work
                    [a3 a3 a3]      # doesn't work
                    [c c+]          # doesn't work
                    [$D $D+]        # doesn't work
                    [$E $EWrap+]    # workaround
                    [$F ($F)+]      # this workaround is not working
                    [($G) $G+]      # this too
                $D: d
                $EWrap: $E
                $E: e
                $F: f
                $G: g
        )");

        tester.TestHasMatch({
            {"a1 a1", true},
            {"a2 a2", false}, // error
            {"a3 a3 a3", false}, // error
            {"c c", false}, // error
            {"d d", false}, // error
            {"e e", true}, // good
            {"f f", false}, // error
            {"g g", false}, // error
        });
    }
}
