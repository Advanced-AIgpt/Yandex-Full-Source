#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart05_Fillers) {

    Y_UNIT_TEST(Filler) {
        TGranetTester tester(R"(
            form f:
                root:
                    a b
                    a $B c
                filler:
                    x
                    y z
            $B:
                1 2 3
            $C:
                %fillers off
                4 5 6
        )");

        tester.TestHasMatch({
            {"a b", true},
            {"a x b", true},
            {"y z a b", true},
            {"y z a 1 2 3 x c y z", true},
            {"a 1 2 x 3 c", true}, // Filler enabled for $B

            {"", false},
            {"x", false},
            {"a x", false},
            {"a b y", false}, // Filler partial phrase
            {"a 4 5 x 6 c", false}, // Filler disabled for $C
        });
    }

    Y_UNIT_TEST(FillerDoesntHideTheTruth) {
        TGranetTester tester(R"(
            form f:
                root:
                    a b
                    a $B c
                filler:
                    a | 1 2
            $B:
                1 2 3
        )");

        tester.TestHasMatch({
            {"a b", true},
            {"a a b", true},
            {"a 1 2 3 c", true}
        });
    }

    Y_UNIT_TEST(AutoFillerIsDisabledByDefault) {
        TGranetTester tester(R"(
            form f:
                root: привет
        )");
        tester.AddEntity("nonsense", "алиса");

        tester.TestHasMatch({
            {"привет", true},
            {"алиса привет", false},
            {"алиса привет алиса", false},
            {"алиса", false},
        });
    }

    Y_UNIT_TEST(AutoFillerOn) {
        TGranetTester tester(R"(
            form f:
                enable_auto_filler: true
                root: привет
        )");
        tester.AddEntity("nonsense", "алиса");

        tester.TestHasMatch({
            {"привет", true},
            {"алиса привет", true},
            {"алиса привет алиса", true},
            {"алиса", false},
        });
    }

    Y_UNIT_TEST(AutoFillerOff) {
        TGranetTester tester(R"(
            form f:
                enable_auto_filler: false
                root: привет
        )");
        tester.AddEntity("nonsense", "алиса");

        tester.TestHasMatch({
            {"привет", true},
            {"алиса привет", false},
            {"алиса привет алиса", false},
            {"алиса", false},
        });
    }

    Y_UNIT_TEST(PASkillsAutoFillerIsEnabledByDefault) {
        TGranetTester tester(R"(
            form f:
                root: привет
                filler: эй
        )", {.IsPASkills = true});
        tester.AddEntity("YANDEX.NONSENSE", "алиса");

        tester.TestHasMatch({
            {"привет", true},
            {"алиса привет", true},
            {"эй привет", true},
            {"эй алиса привет алиса", true},
            {"эй алиса", false},
        });
    }

    Y_UNIT_TEST(PASkillsAutoFillerCollision) {
        TGranetTester tester(R"(
            form f:
                enable_auto_filler: true
                root: привет
                filler: $YANDEX.NONSENSE; эй
        )", {.IsPASkills = true});
        tester.AddEntity("YANDEX.NONSENSE", "алиса");

        tester.TestHasMatch({
            {"привет", true},
            {"алиса привет", true},
            {"эй привет", true},
            {"эй алиса привет алиса", true},
            {"эй алиса", false},
        });
    }

    Y_UNIT_TEST(PASkillsAutoFillerOff) {
        TGranetTester tester(R"(
            form f:
                enable_auto_filler: false
                root: привет
                filler: эй
        )", {.IsPASkills = true});
        tester.AddEntity("YANDEX.NONSENSE", "алиса");

        tester.TestHasMatch({
            {"привет", true},
            {"алиса привет", false},
            {"эй привет", true},
            {"эй алиса привет алиса", false},
            {"эй алиса", false},
        });
    }

    Y_UNIT_TEST(OptimizerIssue) {
        TGranetTester tester(R"(
            form f:
                root:
                    hello? hello? hello? hello? talk talk*
                filler:
                    alice
        )");

        tester.TestHasMatch({
            {"alice talk", true},
        });
    }

    Y_UNIT_TEST(RepeatAfterMeIssue) {
        TGranetTester tester(R"(
            form repeat_after_me:
                slots:
                    request:
                        source: $Request
                        type: string
                root:
                    повторяй $MyWords? $Request
                filler:
                    привет | уже
            $Request:   %cover_fillers | .+ | слово
            $MyWords:   текст
        )");

        tester.TestTagger("repeat_after_me", true, "повторяй уже текст 'привет'(request:)");
    }
}
