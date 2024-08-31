#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart10_Ambiguouty) {

    Y_UNIT_TEST(SingleForm) {
        TGranetTester tester(R"(
            form F:
                slots:
                    SlotX:
                        type: string
                        source: $X
                    SlotA:
                        type: string
                        source: $A
                    SlotB:
                        type: string
                        source: $B
                    SlotABC:
                        type: string
                        source: $ABC
                root: $X | $A | $B | $ABC | .
            $X:   a | b | c | d | e | f
            $A:   a
            $B:   b
            $ABC: a | b | c
        )");

        tester.TestTagger("F", true, "'a'(SlotA:)");
        tester.TestTagger("F", true, "'b'(SlotB:)");
        tester.TestTagger("F", true, "'c'(SlotABC:)");
        tester.TestTagger("F", true, "'d'(SlotX:)");
        tester.TestTagger("F", true, "'e'(SlotX:)");
        tester.TestTagger("F", true, "'f'(SlotX:)");
        tester.TestTagger("F", true, "y"); // -> .
        tester.TestTagger("F", false, "a a"); // no match
    }

    Y_UNIT_TEST(NegativeSimple) {
        TGranetTester tester(R"(
            form f:
                root:
                    .*
                    %negative
                    .* e .*
        )");
        tester.TestHasMatch({
            {"x", true},        // -> .*
            {"x x x", true},    // -> .*
            {"x e x", false}    // -> %negative .* e .*
        });
    }

    Y_UNIT_TEST(DuplicatedRule) {
        TGranetTester tester(R"(
            form f:
                root:
                    a
                    a
                    b
        )");
        tester.TestHasMatch({
            {"a", true},
            {"b", true},
            {"a a", false},
            {"a b", false},
            {"", false}
        });
    }

    Y_UNIT_TEST(RemoveRule) {
        // Weight of negative rule greater than positive
        TGranetTester tester(R"(
            form f:
                root:
                    a
                    b
                    %negative
                    b
        )");
        tester.TestHasMatch({
            {"a", true},
            {"b", false}
        });
    }

    Y_UNIT_TEST(NegativePlain) {
        TGranetTester tester(R"(
            form f:
                root:
                    .*
                    a
                    b c
                    d
                    %negative
                    e
                    f g
                    .* b .*
        )");
        tester.TestHasMatch({
            {"a",       true},  // -> a
            {"e",       false}, // -> %negative e
            {"f g",     false}, // -> %negative f g
            {"f g g",   true},  // -> .*
            {"b c",     true},  // -> b c
            {"d",       true},  // -> d
            {"x",       true},  // -> .*
            {"x x x",   true},  // -> .*
            {"x b x",   false}  // -> %negative .* b .*
        });
    }

    Y_UNIT_TEST(NegativeOverPositive) {
        TGranetTester tester(R"(
            form f:
                root:
                    a
                    a c
                    a .
                    %negative
                    a
                    b
                    .? a .
                    %positive
                    b
        )");
        tester.TestHasMatch({
            {"a", false},       // -> %negative a
            // p1 = prob(parsing "a c" -> "a c") = prob(rule "a c")
            // p2 = prob(parsing "a c" -> "a .") = prob(rule "a .") * prob("a" -> ".")
            // prob(rule "a .") = 2 * prob(rule "a c")     - because of %negative
            // prob("a" -> ".") = e^10
            // hence p1 > p2
            {"a c", true},      // -> a c
            {"a b", true},      // -> a .
            {"b", false},       // -> %negative b
            {"", false}
        });
    }

    /* FIXME(samoylovboris)
    Y_UNIT_TEST(Ambiguity) {
        TGranetTester tester(R"(
            form f:
                keep_variants: true
                root:
                    a b c .*
                    a .* d .+
                    a .* e
        )");

        tester.TestVariantCount("", 0);
        tester.TestVariantCount("a b c", 1);
        tester.TestVariantCount("a b c 1", 1);
        tester.TestVariantCount("a b c d", 1);
        tester.TestVariantCount("a b c d 1", 2);
        tester.TestVariantCount("a b c d 1 2", 2);
        tester.TestVariantCount("a b c d e", 3);
        tester.TestVariantCount("a b c d e f", 2);
        tester.TestVariantCount("a b 1 1 e", 1);

        tester.TestVariantCount("a d", 0);
        tester.TestVariantCount("a b", 0);
        tester.TestVariantCount("a b d", 0);
    }

    Y_UNIT_TEST(Music) {
        NCompiler::TResourceDataLoader reader;
        TGranetTester tester(CompileGrammarFromPath("/granet/music.grnt", LANG_RUS, &reader));
        tester.TestVariantCount("Поставь спят усталые игрушки", 1);
        tester.TestVariantCount("Поставь песню здесь любой текст", 1); // FIXME(samoylovboris)
        tester.TestVariantCount("Поставь песню спят усталые игрушки", 2);
        tester.TestVariantCount("Поставь песню спят усталые", 1);
        tester.TestVariantCount("Поставь спят усталые", 0);
    }
    */

    Y_UNIT_TEST(Force) {
        TGranetTester tester(R"(
            form f:
                root:
                    $A+

            $A:
                a $B*       # (1)

                %negative
                a bb .*     # (2)

                %force_negative
                a bbb .*    # (3)

                %negative
                a b b bbbb  # (4)

                %force_positive
                a . b bbbb  # (5)

            $B:
                b
                bb
                bbb
                bbbb
        )");

        tester.TestHasMatch({
            {"a",               true},  // Matched only rule 1.
            {"a b",             true},  // Matched only rule 1.
            {"a b bb b bbb",    true},  // Matched only rule 1.

            // Matched rules 1 and 2. Probabilities: p(2) > p(1). Winner: 2.
            // Negative rule 2 wins because its match has greater probability
            {"a bb",            false},

            // Matched rules 1 and 2. Probabilities: p(1) > p(2). Winner: 1.
            // Rule 1 wins because its match has greater probability.
            {"a bb b",          true},

            // Matched rules 1 and 3. Probabilities: p(1) > p(3). Winner: 3.
            // Negative rule 3 wins because it has flag "forced".
            {"a bbb b",         false},

            // Matched rules 1, 4 and 5. Probabilities: p(4) > p(1) > p(5). Winner: 5.
            // Positive rule 5 wins because it has flag "forced".
            {"a b b bbbb",      true},

            // Matched rules 1, 3 and 5. Probabilities: p(1) > p(5) > p(3). Winner: 5.
            // Rules 3 and 5 both have flag "forced". Positive rule 5 wins over rule 3 by probability.
            {"a bbb b bbbb",    true},

            // Same tests as above, but multiple $A

            {"a  a             a bb b", true},
            {"a  a b           a bb b", true},
            {"a  a b bb b bbb  a bb b", true},
            {"a  a bb          a bb b", false},
            {"a  a bb b        a bb b", true},
            {"a  a bbb b       a bb b", false},
            {"a  a b b bbbb    a bb b", true},
            {"a  a bbb b bbbb  a bb b", true},

            {"a             a           ", true},
            {"a b           a b         ", true},
            {"a b bb b bbb  a b bb b bbb", true},
            {"a bb          a bb        ", false},
            {"a bb b        a bb b      ", true},
            {"a bbb b       a bbb b     ", false},
            {"a b b bbbb    a b b bbbb  ", true},
            {"a bbb b bbbb  a bbb b bbbb", true},

            {"a             a bb", false},
            {"a b           a bb", false},
            {"a b bb b bbb  a bb", false},
            {"a bb          a bb", false},
            {"a bb b        a bb", false},
            {"a bbb b       a bb", false},
            {"a b b bbbb    a bb", false},
            {"a bbb b bbbb  a bb", false},
        });
    }

    Y_UNIT_TEST(RulesWithSameChain) {
        // Some chains used more than one rule:
        //   a      - used in rules 1, 4' and 5'
        //   a b    - used in rules 2 and 5"
        //   a c    - used in rules 3 and 7'
        //   a d    - used in rules 4" and 6
        // Rules 1, 2, 3, 6 have weight 1.
        // Rules x' and x" have weight 0.5.
        TGranetTester tester(R"(
            form f:
                root:
                    a       # (1)
                    a b     # (2)
                    a c     # (3)
                    a d?    # ->  a (4') | a d (4")

                    %negative
                    a b?    # ->  a (5') | a b (5")
                    a d     # (6)

                    %force_negative
                    a c e?  # ->  a c (7') | a c e (7")
        )");

        tester.TestHasMatch({
            {"a",   true},  // Matched rules 1, 4' and 5'. Probabilities: p(1) > p(4') == p(5'). Winner: 1.
            {"a b", true},  // Matched rules 2 and 5". Probabilities: p(2) > p(5"). Winner: 2.
            {"a c", false}, // Matched rules 3 and 7'. Probabilities: p(3) > p(7'). But rule 7' is forced. Winner: 7'.
            {"a d", false}, // Matched rules 4" and 6. Probabilities: p(6) > p(4"). Winner: 6.
        });
    }
}
