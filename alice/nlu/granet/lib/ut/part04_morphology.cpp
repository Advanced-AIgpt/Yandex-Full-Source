#include <alice/nlu/granet/lib/ut/granet_tester.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetPart04_Morphology) {

    // ~~~~ Wildcard ~~~~

    Y_UNIT_TEST(Lemma) {
        TGranetTester tester(R"(
            form f:
                root:
                    текст
                    %lemma
                    директива lemma сравнивать по лексемам
                    действие распространяется до конца списка правил
                    или до директивы exact
                    такие слова
                    сорока-воровка
                    %exact
                    строгое сравнение
        )");

        tester.TestHasMatch({
            // default == %exact
            {"текст", true},
            {"тексты", false},

            // %lemma
            {"такие слова", true},
            {"такое слово", true},
            {"таким слов", true},
            {"таким таким", false},
            {"сорока воровка", true},
            {"сорок воровок", true}, // not good
            {"сороку воровку", true},

            // %exact
            {"строгое сравнение", true},
            {"СТРОГОЕ СРАВНЕНИЕ", true},
            {"строгие сравнения", false}
        });
    }

    Y_UNIT_TEST(MotherWashedFrame) {
        TGranetTester tester(R"(
            form f:
                root:
                    [$Mother $Washed $Frame]
            $Mother:
                мама
                папа
                сын
                дочь
            $Washed:
                %lemma
                мыла
                чинил
                сломал
            $Frame:
                %lemma
                раму
                стол
                стул
        )");

        tester.TestHasMatch({
            {"мама мыла раму", true},
            {"раму мыла мама", true},
            {"папа чинил стол", true},
            {"мама чинила стол", true},
            {"мама чинила раму", true},
            {"сын сломал стул", true},
            {"дочь стол сломала", true},

            {"рама мыла маму", false},
            {"дочка стол сломала", false},
            {"раму чинили сыновья", false}
        });
    }

    Y_UNIT_TEST(LemmaModifier) {
        TGranetTester tester(R"(
            form f:
                root:
                    [$Mother $Washed<lemma> раму]
            $Mother:
                мама
                папа
            $Washed:
                мыла
                не сломала
        )");

        tester.TestHasMatch({
            {"мама мыла раму", true},
            {"мама не сломала раму", true},
            {"папа мыл раму", true},
            {"папа не сломал раму", true},
        });
    }

    Y_UNIT_TEST(LemmaModifierRecursive) {
        TGranetTester tester(R"(
            form f:
                root:
                    [$Mother $Did<lemma> раму]
            $Mother:
                мама
                папа
            $Did:
                $Washed
                $Fixed
            $Washed:
                мыла
            $Fixed:
                чинила
                не сломала
        )");

        tester.TestHasMatch({
            {"мама мыла раму", true},
            {"мама чинила раму", true},
            {"мама не сломала раму", true},
            {"папа мыл раму", true},
            {"папа чинил раму", true},
            {"папа не сломал раму", true},
        });
    }

    Y_UNIT_TEST(TurkishLemma) {
        TGranetTester tester(R"(
            form f:
                root:
                    %lemma
                    kamera
        )", {.Lang = LANG_TUR});

        tester.TestHasMatch({
            {"kamera", true},
            {"kamerası", true},

            {"çalışması", false},
        });
    }

    Y_UNIT_TEST(TurkishExact) {
        TGranetTester tester(R"(
            form f:
                root: sağ şerit trafik kazası yaralı var
        )", {.Lang = LANG_TUR});

        tester.TestHasMatch({
            {"sağ şerit trafik kazası yaralı var", true},
        });
    }

    // Names of grammemes see in kernel/lemmer/dictlib/ccl.cpp

    Y_UNIT_TEST(InflectNoun) {
        TGranetTester tester(R"(
            $Object:
                Ёжик в тумане
                Горшочек каши
                24 часа

            form nom:
                root: $Object<g:nom>
            form gen:
                root: $Object<g:gen>
            form dat:
                root: $Object<g:dat>
            form acc:
                root: $Object<g:acc>
            form ins:
                root: $Object<g:ins>
            form abl:
                root: $Object<g:abl>
            form loc:
                root: $Object<g:loc>
        )");

        tester.TestMatchedForms({
            {"Ёжик в тумане",   "nom"},
            {"Ежика в тумане",  "acc,gen"},
            {"Ежику в тумане",  "dat"},
            {"Ежиком в тумане", "ins"},
            {"Ежике в тумане",  "abl,loc"},

            {"Горшочек каши",   "acc,nom"},
            {"Горшочка каши",   "gen"},
            {"Горшочку каши",   "dat"},
            {"Горшочком каши",  "ins"},
            {"Горшочке каши",   "abl,loc"},

            {"24 час",          "acc,nom"}, // error
            {"24 часа",         "gen"},
            {"24 часу",         "dat,loc"},
            {"24 часом",        "ins"},
            {"24 часе",         "abl"},

        });
    }

    Y_UNIT_TEST(InflectNounSingularAndPlural) {
        TGranetTester tester(R"(
            $Object:
                музыкальный коллектив

            form pl.nom:
                root: $Object<g:pl,nom>
            form pl.gen:
                root: $Object<g:pl,gen>
            form pl.dat:
                root: $Object<g:pl,dat>
            form pl.acc:
                root: $Object<g:pl,acc>
            form pl.ins:
                root: $Object<g:pl,ins>
            form pl.loc:
                root: $Object<g:pl,loc>

            form sg.nom:
                root: $Object<g:sg,nom>
            form sg.gen:
                root: $Object<g:sg,gen>
            form sg.dat:
                root: $Object<g:sg,dat>
            form sg.acc:
                root: $Object<g:sg,acc>
            form sg.ins:
                root: $Object<g:sg,ins>
            form sg.loc:
                root: $Object<g:sg,loc>
        )");

        tester.TestMatchedForms({
            {"музыкальные коллективы",      "pl.acc,pl.nom"},   // pl,nom
            {"музыкальных коллективов",     "pl.gen"},          // pl,gen
            {"музыкальным коллективам",     "pl.dat"},          // pl,dat
            {"музыкальные коллективы",      "pl.acc,pl.nom"},   // pl,acc
            {"музыкальными коллективами",   "pl.ins"},          // pl,ins
            {"музыкальных коллективах",     "pl.loc"},          // pl,loc

            {"музыкальный коллектив",       "sg.acc,sg.nom"},   // sg,nom
            {"музыкального коллектива",     "sg.gen"},          // sg,gen
            {"музыкальному коллективу",     "sg.dat"},          // sg,dat
            {"музыкальный коллектив",       "sg.acc,sg.nom"},   // sg,acc
            {"музыкальным коллективом",     "sg.ins"},          // sg,ins
            {"музыкальном коллективе",      "sg.loc"},          // sg,loc
        });
    }

    Y_UNIT_TEST(InflectNounSingularAndPluralUnion) {
        TGranetTester tester(R"(
            $Object:
                этот фильм
                эта серия
                это видео
                этот актёр
                этот лес

            form nom:
                root: мне (нравится|нравятся) $Object<g:nom,sg|nom,pl>
            form gen:
                root: описание $Object<g:gen,sg|gen,pl>
            form dat:
                root: перейди к $Object<g:dat,sg|dat,pl>
            form acc:
                root: запусти $Object<g:acc,sg|acc,pl>
            form ins:
                root: с $Object<g:ins,sg|ins,pl>
            form abl:
                root: об $Object<g:abl,sg|abl,pl>
            form loc:
                root: в $Object<g:loc,sg|loc,pl>
        )");

        tester.TestMatchedForms({
            {"мне нравится",    ""},
            {"описание",        ""},
            {"перейди к",       ""},
            {"запусти",         ""},
            {"с",               ""},
            {"об",              ""},
            {"в",               ""},

            // sg
            {"мне нравится  этот фильм",        "nom"},
            {"описание      этого фильма",      "gen"},
            {"перейди к     этому фильму",      "dat"},
            {"запусти       этот фильм",        "acc"},
            {"с             этим фильмом",      "ins"},
            {"об            этом фильме",       "abl"},
            {"в             этом фильме",       "loc"},

            {"мне нравится  эта серия",         "nom"},
            {"описание      этой серии",        "gen"},
            {"перейди к     этой серии",        "dat"},
            {"запусти       эту серию",         "acc"},
            {"с             этой серией",       "ins"},
            {"об            этой серии",        "abl"},
            {"в             этой серии",        "loc"},

            {"мне нравится  это видео",         "nom"},
            {"описание      этого видео",       "gen"},
            {"перейди к     этому видео",       "dat"},
            {"запусти       это видео",         "acc"},
            {"с             этим видео",        "ins"},
            {"об            этом видео",        "abl"},
            {"в             этом видео",        "loc"},

            {"мне нравится  этот актёр",        "nom"},
            {"описание      этого актера",      "gen"},
            {"перейди к     этому актеру",      "dat"},
            {"запусти       этого актера",      "acc"},
            {"с             этим актером",      "ins"},
            {"об            этом актере",       "abl"},
            {"в             этом актере",       "loc"},

            {"мне нравится  этот лес",          "nom"},
            {"описание      этого леса",        "gen"},
            {"перейди к     этому лесу",        "dat"},
            {"запусти       этот лес",          "acc"},
            {"с             этим лесом",        "ins"},
            {"об            этом лесе",         "abl"}, // abl vs loc
            {"в             этом лесу",         "loc"},

            // pl
            {"мне нравятся  эти фильмы",        "nom"},
            {"описание      этих фильмов",      "gen"},
            {"перейди к     этим фильмам",      "dat"},
            {"запусти       эти фильмы",        "acc"},
            {"с             этими фильмами",    "ins"},
            {"об            этих фильмах",      "abl"},
            {"в             этих фильмах",      "loc"},

            {"мне нравятся  эти серии",         "nom"},
            {"описание      этих серий",        "gen"},
            {"перейди к     этим сериям",       "dat"},
            {"запусти       эти серии",         "acc"},
            {"с             этими сериями",     "ins"},
            {"об            этих сериях",       "abl"},
            {"в             этих сериях",       "loc"},

            {"мне нравятся  это видео",         "nom"},
            {"описание      этого видео",       "gen"},
            {"перейди к     этому видео",       "dat"},
            {"запусти       это видео",         "acc"},
            {"с             этим видео",        "ins"},
            {"об            этом видео",        "abl"},
            {"в             этом видео",        "loc"},

            {"мне нравятся  эти актеры",        "nom"},
            {"описание      этих актеров",      "gen"},
            {"перейди к     этим актерам",      "dat"},
            {"запусти       этих актеров",      "acc"},
            {"с             этими актерами",    "ins"},
            {"об            этих актерах",      "abl"},
            {"в             этих актерах",      "loc"},

            {"мне нравятся  эти леса",          "nom"},
            {"описание      этих лесов",        "gen"},
            {"перейди к     этим лесам",        "dat"},
            {"запусти       эти леса",          "acc"},
            {"с             этими лесами",      "ins"},
            {"об            этих лесах",        "abl"},
            {"в             этих лесах",        "loc"},
        });
    }

    Y_UNIT_TEST(InflectVerb) {
        TGranetTester tester(R"(
            $Object:
                включи
                ищи
                найди
                поищи
            form f:
                root:
                    $Object<g:|inf|ipf|pl|pl,ipf>
                    (можешь|можете) $Object<g:inf>
        )");

        tester.TestHasMatch({
            {"", false},

            {"включи", true},
            {"включить", true},
            {"включи", true},
            {"включай", true},
            {"включите", true},
            {"включайте", true},
            {"можешь включить", true},
            {"можете включить", true},

            {"ищи", true},
            {"искать", true},
            {"ищите", true},
            {"можешь искать", true},
            {"можете искать", true},

            {"найди", true},
            {"найти", true},
            {"найдите", true},
            {"можешь найти", true},
            {"можете найти", true},

            {"поищи", true},
            {"поискать", true},
            {"поищите", true},
            {"можешь поискать", true},
            {"можете поискать", true},
        });
    }

    Y_UNIT_TEST(Cyrillic) {
        TGranetTester tester(R"(
            form f:
                root:
                    $Create<g:|непрош,мн|непрош,ед|инф|пов>
            $Create:
                создай
        )");

        // You can check your expression for inflector by util dump_lang:
        //   cd $ARCADIA/alice/nlu/granet/tools/dump_lang
        //   ya make -r
        //   ./dump_lang granet-inflector -v --grams '|непрош,мн|непрош,ед|инф|пов' --text 'создай'
        tester.TestHasMatch({
            {"создай", true},
            {"создадите", true}, // inpraes pl
            {"создашь", true}, // inpraes sg
            {"создать", true}, // inf
            {"создай", true}, // imper
            {"создаёшь", false},
        });
    }

    Y_UNIT_TEST(InflectorIssue1) {
        TGranetTester tester(R"(
            form tv:
                root:
                    $Tv<g:sg>
            $Tv:
                (на|в)? телевизоре
        )");
        tester.TestHasMatch("телевизоре", true);
    }

    Y_UNIT_TEST(InflectorIssue2) {
        TGranetTester tester(R"(
            form done:
                root:
                    $A<g:praet,f|praet,m|praet,pl>
            $A:
                $B
            $B:
                приготовить
        )");
        tester.TestHasMatch("приготовил", true);
    }

    Y_UNIT_TEST(NameLemmatizationIssue) {
        TGranetTester tester(R"(
            form f:
                lemma: true
                root:
                    $Call $Name
            $Call:
                нужен
                позови
                позвони
            $Name:
                [Антон Тимов]
                [Валентин Кузнецов]
                [Валентина Попова]
        )");
        tester.TestHasMatch({
            {"Нужен Антон Тимов", true},
            {"Позови Антона Тимова", true},
            {"Позвони Антону Тимову", true},

            {"Нужен Валентин Кузнецов", true},
            {"Позови Валентина Кузнецова", true},
            {"Позвони Валентину Кузнецову", true},

            {"Нужна Валентина Попова", true},
            {"Позови Валентину Попову", true},
            {"Позвони Валентине Поповой", true},

            {"Нужен Валентин Попов", true}, // not good
        });
    }

    Y_UNIT_TEST(LemmaAsIs) {
        TGranetTester tester(R"(
            form f:
                root:
                    $Call $Name
            $Call:
                %lemma
                нужен
                позови
                позвони
            $Name:
                %lemma_as_is
                [Антон Тимов]
                [Валентин Кузнецов]
                [Валентина Попова]
        )");
        tester.TestHasMatch({
            {"Нужен Антон Тимов", true},
            {"Позови Антона Тимова", true},
            {"Позвони Антону Тимову", true},

            {"Нужен Валентин Кузнецов", true},
            {"Позови Валентина Кузнецова", true},
            {"Позвони Валентину Кузнецову", true},

            {"Нужна Валентина Попова", true},
            {"Позови Валентину Попову", true},
            {"Позвони Валентине Поповой", true},

            {"Нужен Валентин Попов", false}, // good
        });
    }

    void TestAddressBook(TStringBuf lemmaOption) {
        TString grammar = R"(
            form f:
                lemma: true
                slots:
                    item_name:
                        source: $Name
                        type: custom.address_book.item_name
                root:
                    [$Call $Name]
                $Call:
                    набери
                    вызови
                    звони
                    звонок
                    звякни
                $Name:
                    $custom.address_book.item_name

            entity custom.address_book.item_name:
                # This tag will be replaced by lemma or lemma_as_is
                <<LEMMA_OPTION>>: true
                values:
                    anton_timov:        [Антон Тимов] | Тимов
                    andrej_myaskov:     [Андрей Мясков] | Мясков
                    zenya_oparina:      [Женя Опарина] | Опарина
                    roma_sivakov:       [Рома Сиваков] | Сиваков
                    pavel_salomatov:    [Павел Саломатов] | Саломатов
                    ksyusha_gavrilova:  [Ксюша Гаврилова] | Гаврилова
                    ira_gracheva:       [Ира Грачева] | Грачева
                    lera_goleva:        [Лера Голева] | Голева
        )";
        SubstGlobal(grammar, "<<LEMMA_OPTION>>", lemmaOption);
        TGranetTester tester(grammar);

        tester.TestHasMatch({
            {"антона тимова набери", true},
            {"вызови мяскова", true},
            {"звони опариной", true},
            {"звони роме сивакову", true},
            {"звони саломатову павлу", true},
            {"звонок гавриловой ксюше", true},
            {"звякни грачевой", true},
            {"звонок голевой", false}, // bad
        });

        tester.TestTagger("f", true, "вызови 'мяскова'(item_name/custom.address_book.item_name:andrej_myaskov)");
        tester.TestTagger("f", true, "звони 'опариной'(item_name/custom.address_book.item_name:zenya_oparina)");
        tester.TestTagger("f", true, "звони 'роме сивакову'(item_name/custom.address_book.item_name:roma_sivakov)");
        tester.TestTagger("f", true, "звони 'саломатову павлу'(item_name/custom.address_book.item_name:pavel_salomatov)");
        tester.TestTagger("f", true, "звонок 'гавриловой ксюше'(item_name/custom.address_book.item_name:ksyusha_gavrilova)");
        tester.TestTagger("f", true, "звякни 'грачевой'(item_name/custom.address_book.item_name:ira_gracheva)");

        // Should be:
        // tester.TestTagger("f", true, "звонок 'голевой'(item_name/custom.address_book.item_name:lera_goleva)");
        tester.TestTagger("f", false, "звонок голевой");
    }

    Y_UNIT_TEST(NameLemmatizationIssue_DIALOG_7831_lemma) {
        TestAddressBook("lemma");
    }

    Y_UNIT_TEST(NameLemmatizationIssue_DIALOG_7831_lemma_as_is) {
        TestAddressBook("lemma_as_is");
    }

    Y_UNIT_TEST(DisableGlobalLemma) {
        TGranetTester tester(R"(
            form f:
                lemma: true
                root:
                    сравнение по лемме 1
                    %lemma off
                    точное сравнение 1
                    $A
                    %lemma on
                    сравнение по лемме 2
                    $B
            $A:
                сравнение по лемме 3
                %lemma off
                точное сравнение 2
            $B:
                сравнение по лемме 4
                %exact
                точное сравнение 3

        )");
        tester.TestHasMatch({
            {"сравнение по лемме 1", true},
            {"сравнение по лемме 2", true},
            {"сравнение по лемме 3", true},
            {"сравнение по лемме 4", true},
            {"точное сравнение 1", true},
            {"точное сравнение 2", true},
            {"точное сравнение 3", true},

            {"сравнения по леммам 1", true},
            {"сравнения по леммам 2", true},
            {"сравнения по леммам 3", true},
            {"сравнения по леммам 4", true},
            {"точные сравнения 1", false},
            {"точные сравнения 2", false},
            {"точные сравнения 3", false},
        });
    }

    Y_UNIT_TEST(InflectGendersIssue) {
        // List of grammeme names is here: kernel/lemmer/dictlib/ccl.cpp
        TGranetTester tester(R"(
            form f:
                root:
                    с граммемами $Was<g:|f|m|n|pl>
                    с суффиксом $Was<inflect_genders>
                    с директивой $WasWithDirective
                    странный род $Was<g:mf>
            $Was:
                был
            $WasWithDirective:
                %inflect_genders
                был
        )");

        tester.TestHasMatch({
            {"с граммемами был", true}, // m
            {"с граммемами была", true}, // f
            {"с граммемами было", true}, // n
            {"с граммемами были", true}, // pl
            {"с граммемами быть", false},
            {"с граммемами есть", false},

            {"с суффиксом был", true},
            {"с суффиксом была", true},
            {"с суффиксом было", true},
            {"с суффиксом были", true},
            {"с суффиксом быть", false},
            {"с суффиксом есть", false},

            {"с директивой был", true},
            {"с директивой была", true},
            {"с директивой было", true},
            {"с директивой были", true},
            {"с директивой быть", false},
            {"с директивой есть", false},

            {"странный род был", false},
            {"странный род была", false},
            {"странный род было", false},
            {"странный род были", false},
            {"странный род быть", false},
            {"странный род есть", true}, // error?
        });
    }

    Y_UNIT_TEST(InflectGendersIssue2) {
        TGranetTester tester(R"(
            form f:
                root:
                    $TensePast это прошедшее время
                    $TensePresent это настоящее время
                    $TenseFuture это будущее время
            $TensePast:
                %inflect_genders
                был
                наступил
                случилось
                произошла
            $TensePresent:
                %inflect_genders
                есть
                сейчас
            $TenseFuture:
                %inflect_genders
                будет
                наступит
                произойдет
                случится
        )");

        tester.TestHasMatch({
            {"был это прошедшее время", true},
            {"был это настоящее время", true}, // error
            {"был это будущее время", false},

            {"было это прошедшее время", true},
            {"было это настоящее время", false},
            {"было это будущее время", false},

            {"есть это прошедшее время", false},
            {"есть это настоящее время", true},
            {"есть это будущее время", false},

            {"будет это прошедшее время", false},
            {"будет это настоящее время", false},
            {"будет это будущее время", true},
        });
    }
}
