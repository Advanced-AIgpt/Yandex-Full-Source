#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart07_Forms) {

    Y_UNIT_TEST(SingleForm) {
        TGranetTester tester(R"(
            form my_form:
                slots:
                    my_slot_b:
                        type: string
                        source: $B
                    my_slot_c:
                        type: string
                        source: $C
                root:
                    a
                    a $B
                    a $C
            $B:
                b
            $C:
                c
        )");

        tester.TestTagger("my_form", true, "a");
        tester.TestTagger("my_form", true, "a 'b'(my_slot_b/string:b)");
        tester.TestTagger("my_form", true, "a 'c'(my_slot_c/string:c)");
        tester.TestTagger("my_form", false, "c");
    }

    Y_UNIT_TEST(SlotAndDataType) {
        TGranetTester tester(R"(
            form F:
                slots:
                    s.a:
                        type: t.a
                        source: $A
                    s.b:
                        type: t.b
                        source: $B
                root:
                    x $A
                    $A $B y
            $A:
                %type "t.a"
                %value "v.a1"
                11
                a 1
                %value "v.a2"
                2 2 2
                a 2
            $B:
                %type "t.b"
                %value "v.b1"
                11111
                b 1
                %value "{\"json\": \"value\"}"
                2 2 2 2
                b 2
        )");

        tester.TestTagger("F", true, "x '11'(s.a/t.a:v.a1)");
        tester.TestTagger("F", true, R"('a 2'(s.a/t.a:v.a2) 'b 2'(s.b/t.b:{"json": "value"}) y)");
    }

    Y_UNIT_TEST(Fillers) {
        TGranetTester tester(R"(
            form F:
                slots:
                    slot_a:
                        type: string
                        source: $A
                    slot_b:
                        type: string
                        source: $B
                root:
                    111 $A 222 $B 333
            $A:
                aaa
                a1 a2
            $B:
                %cover_fillers
                bbb
                b1 b2
            filler:
                g
        )");

        tester.TestTagger("F", true, "111 'aaa'(slot_a/string:aaa) 222 'bbb'(slot_b/string:bbb) 333");
        tester.TestTagger("F", true, "g 111 g g 'aaa'(slot_a/string:aaa) g 222 'g bbb g g'(slot_b/string:g bbb g g) 333 g");
    }

    Y_UNIT_TEST(SlotConcatenateStrings) {
        UNIT_ASSERT_NO_EXCEPTION(TGranetTester{R"(
            form search:
                enable_alice_tagger: true
                enable_granet_parser: false
                slots:
                    search_text:
                        type: string
                        concatenate_strings: true
        )"});

        UNIT_ASSERT_NO_EXCEPTION(TGranetTester{R"(
            form search:
                enable_alice_tagger: true
                enable_granet_parser: false
                slots:
                    search_text:
                        type: string
                        concatenate_strings: false
        )"});

        UNIT_ASSERT_NO_EXCEPTION(TGranetTester{R"(
            form search:
                enable_alice_tagger: false
                enable_granet_parser: false
                slots:
                    search_text:
                        type: string
                        concatenate_strings: false
        )"});
    }

    Y_UNIT_TEST(SlotKeepVariants) {
        UNIT_ASSERT_NO_EXCEPTION(TGranetTester{R"(
            form search:
                enable_granet_parser: false
                slots:
                    search_text:
                        type: string
                        keep_variants: true
        )"});

        UNIT_ASSERT_NO_EXCEPTION(TGranetTester{R"(
            form search:
                enable_granet_parser: false
                slots:
                    search_text:
                        type: string
                        keep_variants: false
        )"});
    }

    Y_UNIT_TEST(SlotMarkupSimple) {
        TGranetTester tester(R"(
            form play:
                slots:
                    movie:
                        type: string
                root:
                    включи 'кин дза дза'(movie)
                    'кин дза дза'(movie) поставь
            filler:
                алиса
                пожалуйста
        )");

        tester.TestTagger("play", true, "включи 'кин дза дза'(movie)");
        tester.TestTagger("play", true, "включи 'кин дза дза'(movie) пожалуйста");
        tester.TestTagger("play", true, "включи пожалуйста 'кин дза дза'(movie)");
        tester.TestTagger("play", true, "алиса 'кин дза дза'(movie) пожалуйста поставь");
    }

    Y_UNIT_TEST(SlotMarkupInBag) {
        TGranetTester tester(R"(
            form play:
                slots:
                    movie:
                        type: string
                root:
                    [включи 'кин дза дза'(movie)]
            filler:
                алиса
                пожалуйста
        )");

        tester.TestTagger("play", true, "включи 'кин дза дза'(movie)");
        tester.TestTagger("play", true, "включи 'кин дза дза'(movie) пожалуйста");
        tester.TestTagger("play", true, "включи пожалуйста 'кин дза дза'(movie)");
        tester.TestTagger("play", true, "алиса 'кин дза дза'(movie) пожалуйста включи");
    }

    Y_UNIT_TEST(SlotMarkupTyped) {
        TGranetTester tester(R"(
            form play:
                slots:
                    action:
                        type:
                            custom.action
                            string
                    movie:
                        type: string
                root:
                    'включи'(action) 'кин дза дза'(movie)
                    'давай'(action:start) 'кин дза дза'(movie)
                    'давай включи'(action) 'кин дза дза'(movie)
                    'кин дза дза'(movie) 'выключи'(action)
                    'кин дза дза'(movie) 'не надо'(action:stop)
                    'не надо'(action) 'кин дза дза'(movie)
            filler:
                алиса
                пожалуйста
        )");

        tester.AddEntity("custom.action", "включи", "start", -4);
        tester.AddEntity("custom.action", "выключи", "stop", -4);

        tester.TestTagger("play", true, "'включи'(action/custom.action:start) пожалуйста 'кин дза дза'(movie/string:кин дза дза)");
        tester.TestTagger("play", true, "'давай'(action/custom.action:start) 'кин дза дза'(movie/string:кин дза дза)");
        tester.TestTagger("play", true, "'давай включи'(action/custom.action:start) 'кин дза дза'(movie/string:кин дза дза)");

        tester.TestTagger("play", true, "'кин дза дза'(movie/string:кин дза дза) 'выключи'(action/custom.action:stop)");
        tester.TestTagger("play", true, "'кин дза дза'(movie/string:кин дза дза) 'не надо'(action/custom.action:stop)");
        tester.TestTagger("play", true, "'не надо'(action/string:не надо) 'кин дза дза'(movie/string:кин дза дза)");
    }

    Y_UNIT_TEST(SlotMarkupComplex) {
        TGranetTester tester(R"(
            form play:
                slots:
                    movie:
                        type: string
                root:
                    включи фильм? '$Movie'(movie)
                    включай '$Movie'(movie) скорее?
            $Movie:
                кин дза дза
                .*
            filler:
                алиса
                пожалуйста
        )");

        tester.TestTagger("play", true, "включи 'кин дза дза'(movie)");
        tester.TestTagger("play", true, "включи фильм 'кин дза дза'(movie) пожалуйста");
        tester.TestTagger("play", true, "включи фильм 'терминатор два'(movie) пожалуйста");
        tester.TestTagger("play", true, "включай скорее");
    }

    TGranetTester CreateSlotMatchingTypeTester(const char* type) {
        TGranetTester tester(Sprintf(R"(
            form play:
                slots:
                    what:
                        matching_type: %s  # replaced by exact/inside/overlap in test cases
                        type:
                            user.what
                            string
                root:
                    включи песенку '.*'(what)
                    включи фильм '.*'(what)
                    включи фильм 'терминатор 3'(what:terminator3)
        )", type));

        tester.AddEntity("user.what", "терминатор", "terminator1", -4);
        tester.AddEntity("user.what", "терминатор 2", "terminator2", -4);
        tester.AddEntity("user.what", "песенку мамонтёнка", "mamont", -4);
        return tester;
    }

    Y_UNIT_TEST(SlotMatchingTypeExact) {
        TGranetTester tester = CreateSlotMatchingTypeTester("exact");
        tester.TestSlotData("включи песенку мамонтёнка", {"play/what: [string/мамонтёнка]"});
        tester.TestSlotData("включи фильм терминатор",   {"play/what: [user.what/terminator1, string/терминатор]"});
        tester.TestSlotData("включи фильм терминатор 2", {"play/what: [user.what/terminator2, string/терминатор 2]"});
        tester.TestSlotData("включи фильм терминатор 3", {"play/what: [user.what/terminator3]"});
    }

    Y_UNIT_TEST(SlotMatchingTypeInside) {
        TGranetTester tester = CreateSlotMatchingTypeTester("inside");
        tester.TestSlotData("включи песенку мамонтёнка", {"play/what: [string/мамонтёнка]"});
        tester.TestSlotData("включи фильм терминатор",   {"play/what: [user.what/terminator1, string/терминатор]"});
        tester.TestSlotData("включи фильм терминатор 2", {"play/what: [user.what/terminator2, user.what/terminator1, string/терминатор 2]"});
        tester.TestSlotData("включи фильм терминатор 3", {"play/what: [user.what/terminator3]"});
    }

    Y_UNIT_TEST(SlotMatchingTypeOverlap) {
        TGranetTester tester = CreateSlotMatchingTypeTester("overlap");
        tester.TestSlotData("включи песенку мамонтёнка", {"play/what: [user.what/mamont, string/мамонтёнка]"});
        tester.TestSlotData("включи фильм терминатор",   {"play/what: [user.what/terminator1, string/терминатор]"});
        tester.TestSlotData("включи фильм терминатор 2", {"play/what: [user.what/terminator2, user.what/terminator1, string/терминатор 2]"});
        tester.TestSlotData("включи фильм терминатор 3", {"play/what: [user.what/terminator3]"});
    }

    Y_UNIT_TEST(DisableGranetParser) {
        TGranetTester tester(R"(
            form f:
                enable_granet_parser: false
                root: привет
        )");
        tester.TestHasMatch({
            {"привет", false},
        });
    }

    Y_UNIT_TEST(DisableGranetParserNoRoot) {
        TGranetTester tester(R"(
            form f:
                enable_granet_parser: false
        )");
    }

    Y_UNIT_TEST(MultipleSlot) {
        TGranetTester tester(R"(
            form turn_on:
                slots:
                    index:
                        source: $sys.num
                        type: sys.num
                root:
                    включи $sys.num (и? $sys.num)*
        )");
        tester.AddEntity("sys.num", "пять", "5");
        tester.AddEntity("sys.num", "семь", "7");
        tester.AddEntity("sys.num", "двадцать три", "23");

        tester.TestSlotData("включи пять", {
            "turn_on/index: [sys.num/5]"
        });
        tester.TestSlotData("включи пять и семь", {
            "turn_on/index: [sys.num/5]",
            "turn_on/index: [sys.num/7]"
        });
        tester.TestSlotData("включи двадцать три пять и семь", {
            "turn_on/index: [sys.num/23]",
            "turn_on/index: [sys.num/5]",
            "turn_on/index: [sys.num/7]"
        });
    }

    Y_UNIT_TEST(MultipleSlotWithStringType) {
        TGranetTester tester(R"(
            form turn_on:
                slots:
                    index:
                        source: $sys.num
                        type:
                            sys.num
                            string
                root:
                    включи $sys.num (и? $sys.num)*
        )");
        tester.AddEntity("sys.num", "пять", "5");
        tester.AddEntity("sys.num", "семь", "7");
        tester.AddEntity("sys.num", "двадцать три", "23");

        tester.TestSlotData("включи пять", {
            "turn_on/index: [sys.num/5, string/пять]"
        });
        tester.TestSlotData("включи пять и семь", {
            "turn_on/index: [sys.num/5, string/пять]",
            "turn_on/index: [sys.num/7, string/семь]"
        });
        tester.TestSlotData("включи двадцать три пять и семь", {
            "turn_on/index: [sys.num/23, string/двадцать три]",
            "turn_on/index: [sys.num/5, string/пять]",
            "turn_on/index: [sys.num/7, string/семь]"
        });
    }

    Y_UNIT_TEST(NoValue) {
        TGranetTester tester(R"(
            form turn_on:
                slots:
                    index:
                        source: $Index
                        type: sys.num
                root:
                    включи $Index
                $Index:
                    $sys.num
                    %lemma
                    единичка
        )");
        tester.AddEntity("sys.num", "пять", "5");

        tester.TestSlotData("включи пять", {"turn_on/index: [sys.num/5]"});
        tester.TestSlotData("включи единичку", {"turn_on/index: []"});
    }
}
