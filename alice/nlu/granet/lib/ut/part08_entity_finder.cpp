#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart08_EntityFinder) {

    Y_UNIT_TEST(ValuesAsDirectives) {
        TGranetTester tester(R"(
            entity beat:
                root:
                    %value "piano"
                    пианино
                    рояль

                    %value "digital_piano"
                    [цифровое пианино]

                    %value "guitar_chords"
                    [гитарные аккорды]
        )");
        tester.TestEntityFinder({
            "сыграй на 'пианино'(beat:piano)",
            "'пианино цифровое'(beat:digital_piano) давай",

        // By default matching is exact
            "сыграй на рояле",
            "cыграй на цифровом 'пианино'(beat:piano)",
            "больше гитарных аккордов",
        });
    }

    Y_UNIT_TEST(ValuesAsDirectivesContinuousLemma) {
        TGranetTester tester(R"(
            entity beat:
                root:
                    %value "piano"
                    пианино
                    рояль

                    %value "digital_piano"
                    %lemma
                    [цифровое пианино]

                    %value "guitar_chords"
                    [гитарные аккорды]
        )");
        tester.TestEntityFinder({
            "сыграй на 'пианино'(beat:piano)",
            "'пианино цифровое'(beat:digital_piano) давай",

            "сыграй на рояле", // exact (no %lemma)
            "cыграй на 'цифровом пианино'(beat:digital_piano)", // %lemma
            "больше 'гитарных аккордов'(beat:guitar_chords)", // %lemma continuation (confusing)
        });
    }

    Y_UNIT_TEST(ValuesAsKeys) {
        TGranetTester tester(R"(
            entity beat:
                values:
                    "piano":
                        пианино
                        рояль
                    "digital_piano":
                        %lemma
                        [цифровое пианино]
                    "guitar_chords":
                        [гитарные аккорды]
        )");
        tester.TestEntityFinder({
            "сыграй на 'пианино'(beat:piano)",
            "'пианино цифровое'(beat:digital_piano) давай",

            "сыграй на рояле", // exact (no %lemma)
            "cыграй на 'цифровом пианино'(beat:digital_piano)", // %lemma
            "больше гитарных аккордов", // exact (no %lemma)
        });
    }

    Y_UNIT_TEST(ValuesAsKeysKeepOverlapped) {
        TGranetTester tester(R"(
            entity beat:
                keep_overlapped: true
                values:
                    piano:
                        пианино
                        рояль
                    digital_piano:
                        %lemma
                        [цифровое пианино]
                    guitar_chords:
                        [гитарные аккорды]
        )");
        tester.TestEntityFinder({
            "сыграй на 'пианино'(beat:piano)",
            "''пианино'(beat:piano) цифровое'(beat:digital_piano) давай",
        });
    }

    Y_UNIT_TEST(ValuesAsKeysKeepCoincident) {
        TGranetTester tester(R"(
            entity contact:
                keep_overlapped: true
                lemma: true
                values:
                    ivanov:
                        вова
                        иванов
                        [вова иванов]
                    petrov:
                        вова
                        петров
                        [вова петров]
        )");
        tester.TestEntityFinder({
            "позвони ''вове'(contact:ivanov)'(contact:petrov)",
            "позвони '''вове'(contact:ivanov)'(contact:petrov) 'петрову'(contact:petrov)'(contact:petrov)",
        });
    }

    Y_UNIT_TEST(ParameterLemma) {
        TGranetTester tester(R"(
            entity beat:
                lemma: true
                values:
                    guitar_chords:
                        [аккорды на гитаре]
                    piano_chords:
                        [аккорды на? (пианино|рояле|фортепиано)]
        )");
        tester.TestEntityFinder({
            "дай 'аккорды на гитаре'(beat:guitar_chords)",
            "дай 'аккорд на гитаре'(beat:guitar_chords)",
            "дай 'аккорды на рояле'(beat:piano_chords)",
            "дай 'рояль аккорд'(beat:piano_chords)",
        });
    }

    Y_UNIT_TEST(ParameterInflectCases) {
        TGranetTester tester(R"(
            entity route:
                inflect_cases: true
                values:
                    auto:       автомобиль; машина; тачка; автомобильный; автомобильная дорога
                    direct:     по прямой; напрямую; самолет
                    pedestrian: пешком; пешеходный; пеший; ноги
                    transport:  транспорт; общественный транспорт
        )");
        tester.TestEntityFinder({
            "маршрут на 'тачке'(route:auto)",
            "маршрут 'автомобилем'(route:auto)",
            "по 'автомобильной дороге'(route:auto)",
            "маршрут 'автомобильный'(route:auto) дорога платная",
            "маршрут 'самолетом'(route:direct)",
            "на 'общественном транспорте'(route:transport)",
            "'общественным транспортом'(route:transport)",
            // Plural
            "на машинах",
            "по 'автомобильным'(route:auto) дорогам",
            "автомобильными дорогами",
            // Gender
            "траектория автомобильная",
        });
    }

    Y_UNIT_TEST(ParameterInflectNumbers) {
        TGranetTester tester(R"(
            entity route:
                inflect_cases: true
                inflect_numbers: true
                values:
                    auto:       автомобиль; машина; тачка; автомобильный; автомобильная дорога
                    direct:     по прямой; напрямую; самолет
                    pedestrian: пешком; пешеходный; пеший; ноги
                    transport:  транспорт; общественный транспорт
        )");
        tester.TestEntityFinder({
            "маршрут на 'тачке'(route:auto)",
            "маршрут 'автомобилем'(route:auto)",
            "по 'автомобильным дорогам'(route:auto)",
            "маршрут 'автомобильный'(route:auto) дорога платная",
            "маршрут 'самолетом'(route:direct)",
            "на 'общественном транспорте'(route:transport)",
            "'общественным транспортом'(route:transport)",
            // Plural
            "на 'машинах'(route:auto)",
            "по 'автомобильным дорогам'(route:auto)",
            "'автомобильными дорогами'(route:auto)",
            // Gender
            "траектория автомобильная",
        });
    }

    Y_UNIT_TEST(ParameterInflectGenders) {
        TGranetTester tester(R"(
            entity route:
                inflect_genders: true
                values:
                    auto: автомобиль; машина; тачка; автомобильный; автомобильная дорога
        )");
        /* TODO(samoylovboris) Fix inflect_genders
        tester.TestEntityFinder({
            "дорога 'автомобильная'(route:auto)",
        });
        */
    }

    Y_UNIT_TEST(ParameterInflectAll) {
        TGranetTester tester(R"(
            entity genre:
                inflect_cases: true
                inflect_numbers: true
                inflect_genders: true
                values:
                    african:        африканская; [африканская народная]
                    alternative:    $Alternative
                    videogame:      [компьютерных игрушек]; [компьютерных игр]
                    symphonic:      симфоническая $Music  # TODO
                    classic:        [классическая $Music+]  # TODO
            $Alternative:
                альтернатива; альтернативная
            $Music:
                музыка; оркестр
        )");
        tester.TestEntityFinder({
            "включи 'альтернативный'(genre:alternative) рок",
            "включи 'народные африканские'(genre:african) песнопения",
            "включи музыку из 'компьютерной игры'(genre:videogame)",
            "включи 'симфоническую музыку'(genre:symphonic)",
            "включи 'классическую музыку'(genre:classic)"
        });
    }

    Y_UNIT_TEST(ElementInflectAll) {
        TGranetTester tester(R"(
            entity genre:
                root: $Genre<inflect_cases><inflect_numbers><inflect_genders>

            $Genre:
                %type genre

                %value african
                африканская
                [африканская народная]

                %value alternative
                $Alternative

                %value videogame
                [компьютерных игрушек]
                [компьютерных игр]

                %value symphonic
                симфоническая $Music  # TODO

                %value classic
                [классическая $Music+]  # TODO

            $Alternative:
                альтернатива; альтернативная
            $Music:
                музыка; оркестр
        )");
        tester.TestEntityFinder({
            "включи 'альтернативный'(genre:alternative) рок",
            "включи 'народные африканские'(genre:african) песнопения",
            "включи музыку из 'компьютерной игры'(genre:videogame)",
            "включи 'симфоническую музыку'(genre:symphonic)",
            "включи 'классическую музыку'(genre:classic)"
        });
    }

    Y_UNIT_TEST(Basic) {
        TGranetTester tester(R"(
            entity e:
                root:
                    a
                    %value v1
                    11
                    %value v2
                    a 2
                    3 3 3
        )");
        tester.TestEntityFinder({
            "'a 2'(e:v2)",
            "'a'(e:) b 'a 2'(e:v2)",
            "x '11'(e:v1) y z",
            "'a'(e:) b 'a 2'(e:v2) cc dd",
            "b '11'(e:v1) cc dd",
            "'3 3 3'(e:v2) cc '3 3 3'(e:v2) dd",
        });
    }

    Y_UNIT_TEST(KeepOverlapped) {
        TGranetTester tester(R"(
            entity e:
                keep_overlapped: true
                root:
                    a
                    %value v1
                    11
                    %value v2
                    a 2
                    3 3 3
        )");
        tester.TestEntityFinder({
            "''a'(e:) 2'(e:v2)",
            "'a'(e) b ''a'(e) 2'(e:v2)",
            "x '11'(e:v1) y z",
            "'a'(e) b ''a'(e) 2'(e:v2) cc dd",
            "b '11'(e:v1) cc dd",
            "'3 3 3'(e:v2) cc '3 3 3'(e:v2) dd",
        });
    }

    Y_UNIT_TEST(Filler) {
        TGranetTester tester(R"(
            entity e:
                root:
                    a b
            filler:
                x
        )");
        tester.TestEntityFinder({
            "'a b'(e:)",
            "'a x b'(e:)",
            "x x 'a x b'(e:) x x",
            "c 'a x b'(e:) d",
        });
    }

    Y_UNIT_TEST(OldStyle) {
        TGranetTester tester(R"(
            form F:
                slots:
                    slot_a:
                        type: custom.a
                        source: $A
                    slot_b:
                        type: custom.b
                        source: $B
                root:
                    x $A
                    $A $B y
            $A:
                $A.a1
                $A.a2
            $A.a1:
                %type "custom.a"
                %value "value_a1"
                11
                a 1
            $A.a2:
                %type "custom.a"
                %value "value_a2"
                2 2 2
                a 2
            $B:
                $B.b1
                $B.b2
            $B.b1:
                %type "custom.b"
                %value "value_b1"
                11111
                b 1
            $B.b2:
                %type "custom.b"
                %value "{\"json\": \"value\"}"
                2 2 2 2
                b 2
        )");

        tester.TestTagger("F", true, "x '11'(slot_a/custom.a:value_a1)");
        tester.TestTagger("F", true, R"('a 2'(slot_a/custom.a:value_a2) 'b 2'(slot_b/custom.b:{"json": "value"}) y)");
    }

    Y_UNIT_TEST(WithForm) {
        TGranetTester tester(R"(
            form f:
                slots:
                    slot_a:
                        type: custom.a
                        source: $custom.a
                    slot_b:
                        type: custom.b
                        source: $custom.b
                root:
                    x $custom.a
                    $custom.a $custom.b y
            entity custom.a:
                root:
                    %value value_a1
                    11
                    a 1
                    %value value_a2
                    2 2 2
                    a 2
            entity custom.b:
                root:
                    %value "value_b1"
                    11111
                    b 1
                    %value {"key1": 1, "key2": "value"}
                    2 2 2 2
                    b 2
        )");

        tester.TestTagger("f", true, "x '11'(slot_a/custom.a:value_a1)");
        tester.TestTagger("f", true, R"('a 2'(slot_a/custom.a:value_a2) 'b 2'(slot_b/custom.b:{"key1": 1, "key2": "value"}) y)");
    }

    Y_UNIT_TEST(Synonyms) {
        TGranetTester tester(R"(
            entity fio:
                lemma: true
                keep_overlapped: true
                keep_variants: true
                enable_synonyms: all
                values:
                    baba:           Баба
                    babulya:        Бабуля
                    babushka:       Бабушка
                    other:          .
        )");

        tester.TestEntityFinderVariants("fio", "бабушке", {
            "'бабушке'(fio:babushka)",
            "'бабушке'(fio:other)",
        });

        tester.AddEntity("syn.thesaurus_lemma", "бабушке", "бабуля", -1.5);
        tester.AddEntity("syn.thesaurus_lemma", "бабушке", "баба", -1.5);
        tester.TestEntityFinderVariants("fio", "бабушке", {
            "'бабушке'(fio:baba)",
            "'бабушке'(fio:babulya)",
            "'бабушке'(fio:babushka)",
            "'бабушке'(fio:other)",
        });
    }
}
