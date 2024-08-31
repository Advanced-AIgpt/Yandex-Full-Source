#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart08_EntitiesExternal) {

    Y_UNIT_TEST(Simple) {
        TGranetTester tester(R"(
            form f:
                root: $ner.my
        )");

        tester.AddEntity("my", "a");
        tester.AddEntity("my", "b");

        tester.TestHasMatch({
            {"a", true},
            {"b", true},

            {"", false},
            {"c", false},
            {"a a", false},
            {"a b", false},
            {"a c", false},
        });
    }

    Y_UNIT_TEST(MultiTioken) {
        TGranetTester tester(R"(
            form f:
                root:
                    c $ner.my d
                    $ner.my
        )");

        tester.AddEntity("my", "e1 e2 e3");
        tester.AddEntity("my", "e1 e2 e3 e4");
        tester.AddEntity("my", "e1 e2 eee");

        tester.TestHasMatch({
            {"e1 e2 e3", true},
            {"e1 e2 e3 e4", true},
            {"e1 e2 eee", true},
            {"c e1 e2 e3 d", true},
            {"c e1 e2 e3 e4 d", true},
            {"c e1 e2 eee d", true},

            {"e1 e2", false},
            {"e1 e2 e3 d", false},
        });
    }

    Y_UNIT_TEST(Dynamics) {
        TGranetTester tester(R"(
            form f:
                root:
                    c $ner.my d
                    $B $ner.my e3
                $B:
                    b
        )");

        tester.AddEntity("my", "e1 e2");
        tester.AddEntity("my", "e1 e2 e3");

        tester.TestHasMatch({
            {"c e1 e2 d", true},
            {"c e1 e2 e3 d", true},
            {"b e1 e2 e3", true},
            {"b e1 e2 e3 e3", true},

            {"c e1 e2", false},
            {"c e1 e2 e3", false},
            {"b e1 e2 d", false},
            {"b e1 e2 e3 d", false}
        });
    }

    Y_UNIT_TEST(MultipleEntities) {
        TGranetTester tester(R"(
            form f:
                root:
                    $ner.my1 b
                    $ner.my1* $ner.my2
                    $ner.my2* x
        )");

        tester.AddEntity("my1", "1 2");
        tester.AddEntity("my2", "1 2 3");
        tester.AddEntity("my2", "e");

        tester.TestHasMatch({
            {"1 2 b", true},
            {"1 2 3", true},
            {"e", true},
            {"1 2 1 2 3", true},
            {"1 2 e", true},
            {"1 2 1 2 1 2 3", true},
            {"1 2 1 2 e", true},
            {"x", true},
            {"1 2 3 x", true},
            {"e x", true},
            {"1 2 3 e x", true},
            {"e 1 2 3 x", true},
            {"e e x", true},

            {"1 2 1 2", false},
            {"1 2 3 b", false},
            {"1 2 x", false}
        });
    }

    Y_UNIT_TEST(Numbers) {
        TGranetTester tester(R"(
            form f:
                root:
                    включи $Chanel
                $Chanel:
                    1
                    Москва 24
                filler:
                    пожалуйста
        )");

        tester.AddEntity("sys.num", "первый", "1");
        tester.AddEntity("sys.num", "двадцать четыре", "24");

        tester.TestHasMatch({
            {"включи москва 24", true},
            {"включи москва двадцать четыре", true},
            {"включи москва двадцать четыре пожалуйста", true},
            {"включи первый", true},
            {"включи москва двадцать три", false},
            {"включи", false},
        });
    }

    /* FIXME(samoylovboris)
    Y_UNIT_TEST(EntityParams) {
        TGranetTester tester(R"(
            form f:
                root:
                    $ner.my a
                    $ner.my.tag1 b
                    $ner.my.tag2 c
                    $ner.my.tag1.tag2 d
        )");

        tester.AddEntity("my", "1 2", "tag1");
        tester.AddEntity("my", "3 4", "tag2");
        tester.AddEntity("my", "5 6", "tag1;tag2");

        tester.TestHasMatch({
            // $ner.my
            {"1 2 a", true},
            {"3 4 a", true},
            {"5 6 a", true},
            // $ner.my.tag1
            {"1 2 b", true},
            {"3 4 b", false},
            {"5 6 b", true},
            // $ner.my.tag2
            {"1 2 c", false},
            {"3 4 c", true},
            {"5 6 c", true},
            // $ner.my.tag1.tag2
            {"1 2 d", false},
            {"3 4 d", false},
            {"5 6 d", true},
        });
    }
    */
}
