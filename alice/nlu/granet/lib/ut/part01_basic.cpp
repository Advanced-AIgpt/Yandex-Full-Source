#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart01_Basic) {

    // ~~~~ Basic syntax ~~~~

    Y_UNIT_TEST(Empty) {
        TGranetTester tester("");
        tester.TestHasMatch({
            {"a", false},
            {"", false}
        });
    }

    Y_UNIT_TEST(NoFormNoRoot) {
        // No form - no results
        TGranetTester tester(R"(
            $A: a
        )");
        tester.TestHasMatch({
            {"a", false},
            {"b", false},
            {"", false}
        });
    }

    Y_UNIT_TEST(Form) {
        // Form defines starting point for parser.
        // In this case starting point is element 'root'.
        // Element 'B' is not used in grammar.
        TGranetTester tester(R"(
            form my_form:
                root: a
            $B: b
        )");
        tester.TestHasMatch({
            {"a",   true},
            {"b",   false},
            {"aa",  false},
            {"a a", false},
            {"ab",  false},
            {"ba",  false},
            {"",    false}
        });
    }

    Y_UNIT_TEST(NestedElements) {
        TGranetTester tester(R"(
            form my_form:
                root:
                    a
                    $B
                    $C
                $B:
                    b
                $C:
                    c1
            $C:
                c2
        )");
        tester.TestHasMatch({
            {"a",   true},
            {"b",   true},
            {"c1",  true},
            {"c2",  false},
            {"",    false},
        });
    }

    Y_UNIT_TEST(FileLevelRoot) {
        TGranetTester tester(R"(
            form my_form
            root:
                a
                $B
            $B:
                b
        )");

        tester.TestHasMatch({
            {"a",   true},
            {"b",   true},
            {"c",   false},
            {"",    false},
        });
    }

    Y_UNIT_TEST(FormScopes) {
        TGranetTester tester(R"(
            form f1:
                root:
                    a
                    $B
                    $C
                    d
                $C:
                    c1
            form f2:
                root:
                    a
                    $B
                    $C
                    e
                $C:
                    c2
            $B:
                b
            $C:
                c3
        )");

        tester.TestMatchedForms({
            {"a",   "f1,f2"},
            {"b",   "f1,f2"},
            {"c1",  "f1"},
            {"c2",  "f2"},
            {"c3",  ""},
            {"d",   "f1"},
            {"e",   "f2"},
            {"",    ""},
        });
    }

    Y_UNIT_TEST(ElementScopes) {
        TGranetTester tester(R"(
            form f1:
                root:
                    $A
            $A:
                a1 $B
                a2 $B
                $B:
                    b1
                    b2
            $B:
                bb1
                bb2
        )");

        tester.TestHasMatch({
            {"a1 b1",   true},
            {"a1 b2",   true},
            {"a2 b1",   true},
            {"a2 b2",   true},
            {"a1 bb1",  false},
        });
    }

    Y_UNIT_TEST(Normalization) {
        TGranetTester tester(R"(
            form f:
                root: Abc
        )");

        tester.TestHasMatch({
            {"abc", true},
            {"ABC", true},
            {". ABC,-", true},

            {"abc7", false},
            {"a b c", false}
        });
    }

    Y_UNIT_TEST(DoubleWord) {
        TGranetTester tester(R"(
            form f:
                root: some thing
        )");

        tester.TestHasMatch({
            {"some thing", true},
            {"some-thing", true},

            {"something", false},
            {"some some thing", false},
            {"some", false},
            {"thing", false}
        });
    }

    Y_UNIT_TEST(SimpleVariants) {
        TGranetTester tester(R"(
            form f:
                root: aaa | bbb | xyz
        )");

        tester.TestHasMatch({
            {"aaa", true},
            {"bbb", true},
            {"xyz", true},

            {"", false},
            {"c", false},
            {"aaa aaa", false},
            {"aaa bbb", false},
            {"bbb aaa", false},
            {"x", false},
            {"z", false}
        });
    }

    Y_UNIT_TEST(SimpleTree) {
        TGranetTester tester(R"(
            form f:
                root: a | a $B
                $B: b
        )");

        tester.TestHasMatch({
            {"a", true},
            {"a b", true},

            {"ab", false},
            {"b", false}
        });
    }

    Y_UNIT_TEST(DoubleEmpty) {
        // You can add empty rule by $sys.void (see 'B').
        TGranetTester tester(R"(
            form f:
                root: $B $B
                $B: $sys.void
        )");

        tester.TestHasMatch({
            {"", true}
        });
    }

    Y_UNIT_TEST(DoubleEmpty2) {
        // You can add empty rule by $sys.void (see 'B').
        TGranetTester tester(R"(
            form f:
                root: a $B $B
                $B: $sys.void
        )");

        tester.TestHasMatch({
            {"a", true}
        });
    }

    Y_UNIT_TEST(Quoted) {
        TGranetTester tester(R"(
            form f:
                root:
                    "7:00"
                    "23 + 17"
                    "5 $"
                filler:
                    пожалуйста
        )");
        tester.TestHasMatch("7:00", true);
        tester.TestHasMatch("7 00", true);
        tester.TestHasMatch("7:00 пожалуйста", true);
        tester.TestHasMatch("23 + 17", true);
        tester.TestHasMatch("23 17", true); // error
        tester.TestHasMatch("5 $", true);
        tester.TestHasMatch("5", true); // error
        tester.TestHasMatch("6 $", false);
    }
}
