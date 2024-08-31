#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart16_Synonyms) {

    Y_UNIT_TEST(Test) {
        TGranetTester tester(R"(
            form f1:
                lemma: true
                enable_synonyms: all
                root:
                    $Alice $TurnOn $ToCall
            form f2:
                lemma: true
                root:
                    $Alice $TurnOn $ToCall
            form f3:
                root:
                    $Alice $TurnOn $ToCall
            form f4:
                lemma: true
                enable_synonyms: translit
                root:
                    $AliceDiminName $TurnOn $ToCall
            $Alice:
                %disable_synonyms translit,dimin_name
                алиса
            $AliceDiminName:
                %disable_synonyms all
                %enable_synonyms translit,dimin_name
                алиса
            $TurnOn:
                %enable_synonyms all
                включи
            $ToCall:
                %disable_synonyms all
                позвонить
        )");

        tester.AddEntity("syn.thesaurus_lemma", "алиска", "алиса", -1);
        tester.AddEntity("syn.thesaurus_lemma", "вруби", "включать", -1);
        tester.AddEntity("syn.translit_ru_lemma", "alisa", "алиса", -1);

        tester.TestMatchedForms({
            {"алиса включи позвонить", "f1,f2,f3,f4"},
            {"алиса включить позвонить", "f1,f2,f4"},
            {"алиса вруби позвонить", "f1,f2,f4"},
            {"алиска вруби позвонить", "f1"},
            {"алиса включи номер", ""},
            {"alisa вруби позвонить", "f4"}
        });
    }
}

