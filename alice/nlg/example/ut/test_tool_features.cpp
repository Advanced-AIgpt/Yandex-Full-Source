#include <alice/nlg/example/register.h>
#include <alice/nlg/library/testing/testing_helpers.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <util/generic/hash_set.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/testing/unittest/registar.h>
#include <string>

using namespace NAlice;
using namespace NAlice::NNlg::NTesting;

namespace {

const auto REG = &NAlice::NNlg::NExample::RegisterAll;

} // namespace

Y_UNIT_TEST_SUITE(ToolFeatures) {
    Y_UNIT_TEST(Cond) {
        std::pair<TStringBuf, TStringBuf> nameGreetings[] = {
            {"John", "Ladies and gentlemen, The Beatles!"},
            {"Paul", "Ladies and gentlemen, The Beatles!"},
            {"George", "Ladies and gentlemen, The Beatles!"},
            {"Ringo", "Ladies and gentlemen, The Beatles!"},
            {"Elton", "Ladies and gentlemen, The bitch is back!"},
            {"Ozzy", "Ladies and gentlemen, Prince of darkness!"},
            {"Alice", "Ladies and gentlemen, The misterious stranger!"},
        };

        for (auto [name, greeting] : nameGreetings) {
            const auto context = NJson::TJsonMap({
                {"a", NJson::TJsonValue(name)},
            });

            TestPhrase(REG, "cond", "cond", greeting, context);
        }
    }

    Y_UNIT_TEST(MacroImports) {
        TestPhrase(REG, "imports", "test", "Bar! = Bar! baz is Buzz!");
    }

    Y_UNIT_TEST(NlgImport) {
        TestPhrase(REG, "imports", "imported_phrase", "Hello");
    }

    Y_UNIT_TEST(VoiceText) {
        TestPhraseTextVoice(REG, "voice_text", "test", "Текст Текст", "Голос Голос");
    }

    Y_UNIT_TEST(CallBlock) {
        TestPhrase(REG, "call_block", "test", "Hello Hello Hello");
    }

    Y_UNIT_TEST(Choose) {
        THashSet<TString> actual;

        i64 rvIndex = 0;
        TVector<i64> randomValues = {
            0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1,
        };
        TFakeRng rng(TFakeRng::TIntegerTag{}, [&]() { return randomValues.at(rvIndex++); });

        // 2**3 = 8 possible combinations of Hello/Goodbye
        for (size_t i = 0; i < 8; ++i) {
            auto out = GetRenderPhraseResult(*CreateTestingNlgRenderer(REG), "call_block", "choose", &rng);
            UNIT_ASSERT_VALUES_EQUAL(out.Text, out.Voice);
            actual.insert(out.Text);
        }

        THashSet<TString> expected = {
            "Hello Hello Hello",   "Hello Hello Goodbye",   "Hello Goodbye Hello",   "Hello Goodbye Goodbye",
            "Goodbye Hello Hello", "Goodbye Hello Goodbye", "Goodbye Goodbye Hello", "Goodbye Goodbye Goodbye",
        };

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Choose2) {
        THashSet<TString> actual;

        i64 randomValue = 0;
        TFakeRng rng(TFakeRng::TIntegerTag{}, [&]() { return randomValue++; });

        // 10 iterations should be enough, could use vector-based fake RNG
        // to explicitly go over all possible paths but chose the simpler route here
        for (size_t i = 0; i < 10; ++i) {
            auto out = GetRenderPhraseResult(*CreateTestingNlgRenderer(REG), "call_block", "choose2", &rng);
            UNIT_ASSERT_VALUES_EQUAL(out.Text, out.Voice);
            actual.insert(out.Text);
        }

        THashSet<TString> expected = {"Hello", "Goodbye"};

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(Div1Card) {
        auto nlg = CreateTestingNlgRenderer(REG);

        NJson::TJsonValue expected;
        expected["states"] = NJson::TJsonValue(NJson::JSON_ARRAY);
        expected["background"] = NJson::TJsonValue(NJson::JSON_ARRAY);

        TRng rng(4);
        const auto actual = nlg->RenderCard("card", "div1", ELanguage::LANG_RUS, rng, NNlg::TRenderContextData());
        UNIT_ASSERT_VALUES_EQUAL(expected, actual.Card);
    }

    Y_UNIT_TEST(Div2Card) {
        auto nlg = CreateTestingNlgRenderer(REG);

        NJson::TJsonValue expected;
        expected["templates"] = NJson::TJsonValue(NJson::JSON_MAP);
        expected["card"] = NJson::TJsonValue(NJson::JSON_MAP);

        TRng rng(4);
        const auto actual = nlg->RenderCard("card", "div2", ELanguage::LANG_RUS, rng, NNlg::TRenderContextData());
        UNIT_ASSERT_VALUES_EQUAL(expected, actual.Card);
    }

    Y_UNIT_TEST(Literals) {
        TestPhrase(REG, "literals", "test",
                   "True False 1 1.1 [1, 2, 3] {'a': 1, 'b': [2, '3']} {'a': 1, 'b': [2, '3']} foo None");
    }

    Y_UNIT_TEST(Subscript) {
        const auto context = NJson::TJsonMap({{
            {"stuff", NJson::TJsonMap({{{"foo", NJson::TJsonValue("FOO")}}})},
            {"vtor", NJson::TJsonArray({NJson::TJsonValue(123), NJson::TJsonValue(456)})},
            {"foo", NJson::TJsonValue("foo")},
            {"zero", NJson::TJsonValue(0)},
        }});

        TStringBuf expected = "FOO FOO FOO 123 123 123 True False True False";
        TestPhrase(REG, "subscript", "test", expected, context);
    }

    Y_UNIT_TEST(Scope) {
        TestPhrase(REG, "scope", "test",
                   "test:local 1 with:local with:special 1 2 test:local 1 2 test:local 1 with:local with:special 1 2 "
                   "test:local 1 2");
    }

    Y_UNIT_TEST(ForLoop) {
        for (auto phrase : {"test_range1", "test_range2", "test_range3"}) {
            TestPhrase(
                REG, "for_loop", phrase,
                "x c F L i0 i ri0 ri px nx || 0 0 True False 0 1 4 5 1 | 1 1 False False 1 2 3 4 0 2 | 2 2 False "
                "False 2 3 2 3 1 3 | 3 3 False False 3 4 1 2 2 4 | 4 4 False True 4 5 0 1 3 | length = 5");
        }
        TestPhrase(REG, "for_loop", "test_list",
                   "x c F L i0 i ri0 ri px nx || foo foo True False 0 1 4 5 123 | 123 123 False False 1 2 3 4 foo "
                   "None | None None False False 2 3 2 3 123 5.5 | 5.5 5.5 False False 3 4 1 2 None [1, 2, 3] | [1, "
                   "2, 3] [1, 2, 3] False True 4 5 0 1 5.5 | length = 5");
        TestPhrase(REG, "for_loop", "test_undefined", "x c F L i0 i ri0 ri px nx || NOTHING!");
        TestPhrase(REG, "for_loop", "test_list_of_lists",
                   "x c F L i0 i ri0 ri px nx || 1 2 True False 0 1 1 2 ['foo', True] | foo True False True 1 2 0 1 "
                   "[1, 2] | length = 2");
        TestPhrase(REG, "for_loop", "test_nested_loops",
                   "[ [4, 5, 6] 1] 2 1 1 | 1 3 2 2 | 2 3 3 | [[1, 2, 3] 2] 5 1 4 | 4 6 2 5 | 5 3 6 |");

        auto testDictLoop = [](TStringBuf phrase, TStringBuf expected) {
            auto splitOutput = [](TStringBuf str) -> THashSet<TString> {
                THashSet<TString> result;
                for (auto it : StringSplitter(str).Split('|')) {
                    TStringBuf token = it.Token();
                    result.insert(TString{StripString(token)});
                }
                return result;
            };

            auto actual = GetRenderPhraseResult(*CreateTestingNlgRenderer(REG), "for_loop", phrase);
            UNIT_ASSERT_VALUES_EQUAL(actual.Text, actual.Voice);
            UNIT_ASSERT_VALUES_EQUAL(splitOutput(expected), splitOutput(actual.Text));
        };

        testDictLoop("test_dict", "a a | c c | b b | length = 3");
        testDictLoop("test_dict_keys", "a a | c c | b b | length = 3");
        testDictLoop("test_dict_values", "123 123 | 789 789 | 456 456 | length = 3");
        testDictLoop("test_dict_items", "a 123 | c 789 | b 456 | length = 3");
    }

    Y_UNIT_TEST(MacroParams) {
        TestPhrase(REG, "macro_params", "test",
                   "1 2 3 4 | test 2 3 4 || 1 2 -3 4 | test 2 -3 4 || 1 2 -3 -4 | test 2 -3 -4 || -1 -2 -3 -4 | test "
                   "-2 -3 -4 || 1 -2 3 -4 | test -2 3 -4 ||");
    }

    Y_UNIT_TEST(MacroParamsUnused) {
        TestPhrase(REG, "macro_params_unused", "test", "x");
    }

    Y_UNIT_TEST(Filters) {
        TestPhrase(REG, "filters", "test",
                   "1 | 3.14 | 1 | | Привет мир | Привет мир | как дела? | как дела? | 123 | 123 | 1 | | 123.45 | 2 | "
                   "&quot;hello<br/>there&quot; | 123 | 6 | a b c | a, b, c | 3 | | 2 | [0, 1, 2, 3, 4] | ['foo'] | "
                   "привет | [123, 456] | 3.1 | 1 | 2 | М*в* | [1, 2, 3] | [1,2.3,\"4\",[5],null,{\"6\":7}] | [привет] | "
                   "%D0%9F%D1%80%D0%B8%D0%B2%D0%B5%D1%82%20%D0%BC%D0%B8%D1%80 | ПРИВЕТ | Hello there | Hello... | "
                   "Hello... |");
    }

    Y_UNIT_TEST(GlobalVar) {
        TestPhrase(REG, "global_var", "test", "local x = 123 | global x = 1");
    }

    Y_UNIT_TEST(GlobalBuiltin) {
        TFakeRng rng(TFakeRng::TDoubleTag{}, []() { return 0.0; });
        const auto actual = GetRenderPhraseResult(*CreateTestingNlgRenderer(REG, &rng), "global_builtin", "test");
        UNIT_ASSERT_VALUES_EQUAL("Hello", actual.Text);
        UNIT_ASSERT_VALUES_EQUAL("", actual.Voice);
    }

    Y_UNIT_TEST(GlobalFor) {
        TestPhrase(REG, "global_for", "test", "123");
    }

    Y_UNIT_TEST(DoStmt) {
        TestPhrase(REG, "do_stmt", "test", "123");
    }

    Y_UNIT_TEST(Random) {
        TestPhrase(REG, "filter_random", "test", "String: a | List: 1 | Range: 0 |");
    }

    Y_UNIT_TEST(Import) {
        TestPhrase(REG, "import", "test", "1 123");
    }

    Y_UNIT_TEST(ImportIntransitive) {
        TestPhrase(REG, "import_intransitive", "test", "");
    }

    Y_UNIT_TEST(CondExpr) {
        TestPhrase(REG, "cond_expr", "test", "4");
    }

    Y_UNIT_TEST(ImportWithContext) {
        TestPhrase(REG, "globals_import", "test", "No context | With context 123");
    }

    Y_UNIT_TEST(FromImportWithContext) {
        TestPhrase(REG, "globals_from_import", "test", "No context | With context 123");
    }

    Y_UNIT_TEST(NlgImportTransitive) {
        TestPhrase(REG, "nlgimport_bottom", "test", "test");
        TestPhrase(REG, "nlgimport_middle", "test", "test");
        TestPhrase(REG, "nlgimport_top", "test", "test");
    }

    Y_UNIT_TEST(TupleAssignment) {
        TestPhrase(REG, "tuple_assign", "test", "1 3 4");
    }

    Y_UNIT_TEST(BlockAssignment) {
        TestPhrase(REG, "block_assign", "test", "foo = Hello | bar = Hello brave...");
    }

    Y_UNIT_TEST(BlockFilter) {
        TestPhrase(REG, "block_filter", "test", "Hello brave...");
    }

    Y_UNIT_TEST(BlockAssignSelfref) {
        TestPhrase(REG, "block_assign_selfref", "test", "global | local global");
    }

    Y_UNIT_TEST(BlockAssignNested) {
        TestPhrase(REG, "block_assign_nested", "test", "1 2 | 1 3 || 2");
    }

    Y_UNIT_TEST(UnaryOperators) {
        TestPhrase(REG, "unary_operators", "test", "1 5 False True True");
    }

    Y_UNIT_TEST(TruthValue) {
        TestPhrase(REG, "truth_value", "test",
                   "False False True False True False True False True False True False True True False False");
    }

    Y_UNIT_TEST(Operators) {
        TestPhrase(REG, "operators", "test", "True True False -2 3 True aaaaa [] [1, 2, 3, 4, 5, 6] 2 None [1, 2, 3] 1");
    }

    Y_UNIT_TEST(Concat) {
        TestPhrase(REG, "concat", "test", "None1[2, 3]4");
    }

    Y_UNIT_TEST(VoiceTextChoose) {
        THashSet<std::pair<TString, TString>> actual;

        i64 rvIndex = 0;
        TVector<i64> randomValues = {
            0, 0, 0, 1, 1, 0, 1, 1,
        };
        TFakeRng rng(TFakeRng::TIntegerTag{}, [&]() { return randomValues[rvIndex++]; });

        // 2**2 = 4 possible combinations
        for (size_t i = 0; i < 4; ++i) {
            auto out = GetRenderPhraseResult(*CreateTestingNlgRenderer(REG), "voice_text_chooseline", "test", &rng);
            actual.insert({out.Text, out.Voice});
        }

        THashSet<std::pair<TString, TString>> expected = {
            {"Привет Текст Привет Текст", "Привет Привет"},
            {"Привет Текст Здравствуй", "Привет Голос Здравствуй"},
            {"Здравствуй Привет Текст", "Голос Здравствуй Привет"},
            {"Здравствуй Здравствуй", "Голос Здравствуй Голос Здравствуй"},
        };

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(VoiceTextFilters) {
        TestPhraseTextVoice(REG, "voice_text_filters", "test", "Текст direct Текст only_text", "Голос direct Голос only_voice");
    }

    Y_UNIT_TEST(VoiceTextMacros) {
        TestPhraseTextVoice(REG, "voice_text_macros", "test", "Hello Текст foo Hello Текст foo", "Hello 123 Hello foo");
    }

    Y_UNIT_TEST(Slice) {
        TestPhrase(REG, "slice", "test", "[1, 2, 3] [2, 3] [1, 3] [1, 2]");
    }

    Y_UNIT_TEST(ErrorRuntime) {
        TRng rng;
        UNIT_ASSERT_EXCEPTION(GetRenderPhraseResult(*CreateTestingNlgRenderer(REG), "error_runtime", "test", &rng), NNlg::TRuntimeError);
    }

    Y_UNIT_TEST(ChooseItemExplicit) {
        constexpr double weightFoo = 0.5;
        constexpr double weightBar = 1;
        constexpr double weightBaz = 2;
        constexpr double weightTotal = weightFoo + weightBar + weightBaz;

        const TVector<std::pair<TString, double>> expectedProb = {
            {"Foo", weightFoo / weightTotal},
            {"Bar", weightBar / weightTotal},
            {"Baz", weightBaz / weightTotal},
        };
        CheckChoiceFreqs(REG, "chooseitem", "test_explicit", expectedProb);
    }

    Y_UNIT_TEST(ChooseItemDefault) {
        constexpr double weightFoo = 1;
        constexpr double weightBar = 1;
        constexpr double weightBaz = 1;
        constexpr double weightTotal = weightFoo + weightBar + weightBaz;

        const TVector<std::pair<TString, double>> expectedProb = {
            {"Foo", weightFoo / weightTotal},
            {"Bar", weightBar / weightTotal},
            {"Baz", weightBaz / weightTotal},
        };
        CheckChoiceFreqs(REG, "chooseitem", "test_default", expectedProb);
    }

    Y_UNIT_TEST(ChooseItemOne) {
        const TVector<std::pair<TString, double>> expectedProb = {
            {"Foo", 1},
        };
        CheckChoiceFreqs(REG, "chooseitem", "test_one", expectedProb);
    }

    Y_UNIT_TEST(MaybeExplicit) {
        const TVector<std::pair<TString, double>> expectedProb = {
            {"Hello", 0.2},
        };
        CheckChoiceFreqs(REG, "maybe", "test_explicit", expectedProb);
    }

    Y_UNIT_TEST(MaybeDefault) {
        const TVector<std::pair<TString, double>> expectedProb = {
            {"Hello", 0.5},
        };
        CheckChoiceFreqs(REG, "maybe", "test_default", expectedProb);
    }

    Y_UNIT_TEST(WithShadowing) {
        TestPhrase(REG, "with_shadowing", "test", "Hello xxxx");
    }

    Y_UNIT_TEST(AssignSelf) {
        TestPhrase(REG, "assign_self", "test", "123 [1, 2]");
    }

    Y_UNIT_TEST(Tests) {
        TestPhrase(REG, "tests", "test",
                   "False True True True False False False True True True True False False False True False False "
                   "False False False False False False True False False");
    }

    Y_UNIT_TEST(BuiltinMethods) {
        TestPhrase(REG, "builtin_methods", "test",
                   "| 1 | None | [True, True] | [True, True, False] | | [False, True, "
                   "True, True] | | | [1, 2, 3, 4] | True | False | 1, 2, 3 | hello | "
                   "превед Hello-- | ++Hello++ | --Hello | ['1', '2:3'] | ['1', '2', "
                   "'3'] | True | False | Hello HELLO | ПРЕВЕД |");
    }

    Y_UNIT_TEST(IntentName) {
        TestPhrase(REG, "intent_name", "test", "intent_name_ru");
    }

    Y_UNIT_TEST(GlobalMacroCall) {
        TestPhrase(REG, "global_macro_usage", "test", "Hello world!");
    }

    Y_UNIT_TEST(CallerParams) {
        TestPhrase(REG, "caller_params", "test", "1 2 3 2 3 4 3 4 5");
    }

    Y_UNIT_TEST(ActionDirectives) {
        TestPhrase(REG, "action_directives", "test",
                   "CAD: foo {'foo': 1} client_action sub_foo | SAD: bar {'foo': 1} server_action True");
    }

    Y_UNIT_TEST(ExperimentFlag) {
        const auto context = NJson::ReadJsonFastTree(R"({"experiments":{"exp1":"foo","exp2":null}})");
        TestPhrase(REG, "experiment_flag", "test1",
                   "`exp1 in exps`: True "
                   "`exp1 is defined`: True `exp1 is undefined`: False "
                   "`exps['exp1'] == None`: False `exps.get('exp1') == None`: False "
                   "`exps['exp1'] == 'foo'`: True `exps.get('exp1') == 'foo'`: True", context, NJson::TJsonMap());
        TestPhrase(REG, "experiment_flag", "test2",
                   "`exp2 in exps`: True "
                   "`exp2 is defined`: True `exp2 is undefined`: False "
                   "`exps['exp2'] == None`: True `exps.get('exp2') == None`: True "
                   "`exps['exp2'] == 'foo'`: False `exps.get('exp2') == 'foo'`: False", context, NJson::TJsonMap());
        TestPhrase(REG, "experiment_flag", "test3",
                   "`exp3 in exps`: False "
                   "`exp3 is defined`: False `exp3 is undefined`: True "
                   "`exps['exp3'] == None`: False `exps.get('exp3') == None`: True "
                   "`exps['exp3'] == 'foo'`: False `exps.get('exp3') == 'foo'`: False", context, NJson::TJsonMap());
    }

    Y_UNIT_TEST(Form) {
        const auto form = NJson::ReadJsonFastTree(R"({"slots":[{"slot":"foo","value":"bar","type":"baz"}]})");
        TestPhrase(REG, "form", "test", "bar has the type baz", NJson::TJsonMap(), form);
    }

    Y_UNIT_TEST(Inflect) {
        TestPhrase(REG, "inflect", "test", "Брюса Уиллиса чайником каюк каюка");
    }

    Y_UNIT_TEST(TtsDomain) {
        TestPhraseTextVoice(REG, "tts_domain", "test", "05/07/033", "<[domain music]>05/07/033<[/domain]>");
    }

    Y_UNIT_TEST(DatesRu) {
        TestPhrase(
            REG, "dates", "test",
            "7 1 2019 января понедельник | 5 2 2019 февраля вторник | 6 3 2019 марта среда | "
            "11 4 2019 апреля четверг | 10 5 2019 мая пятница | 8 6 2019 июня суббота | "
            "14 7 2019 июля воскресенье | 19 8 2019 августа понедельник | 10 9 2019 сентября вторник | "
            "23 10 2019 октября среда | 28 11 2019 ноября четверг | 6 12 2019 декабря пятница |");
    }

    Y_UNIT_TEST(DatesTr) {
        TestPhrase(
            REG, "dates", "test",
            "7 1 2019 Ocak Pazartesi | 5 2 2019 Şubat Salı | 6 3 2019 Mart Çarşamba | "
            "11 4 2019 Nisan Perşembe | 10 5 2019 Mayıs Cuma | 8 6 2019 Haziran Cumartesi | "
            "14 7 2019 Temmuz Pazar | 19 8 2019 Ağustos Pazartesi | 10 9 2019 Eylül Salı | "
            "23 10 2019 Ekim Çarşamba | 28 11 2019 Kasım Perşembe | 6 12 2019 Aralık Cuma |",
            /* context */ NJson::TJsonMap(),
            /* form */ NJson::TJsonMap(),
            /* reqInfo */ NJson::TJsonMap(),
            ELanguage::LANG_TUR);
    }

    Y_UNIT_TEST(DatesAr) {
        const auto text = TString(
            "7 1 2019 كانون الثاني الاثنين | 5 2 2019 شباط الثلاثاء"
            " | 6 3 2019 آذار الأربعاء | 11 4 2019 نيسان الخميس"
            " | 10 5 2019 أيار الجمعة | 8 6 2019 حزيران السبت"
            " | 14 7 2019 تموز يوم الأحد | 19 8 2019 آب الاثنين"
            " | 10 9 2019 أيلول الثلاثاء | 23 10 2019 تشرين الأول الأربعاء"
            " | 28 11 2019 تشرين الثاني الخميس | 6 12 2019 كانون الأول الجمعة |");
        const auto voice = TString("<speaker voice=\"arabic.gpu\" lang=\"ar\">") + text;
        TestPhraseTextVoice(
            REG, "dates", "test", text, voice,
            /* context */ NJson::TJsonMap(),
            /* form */ NJson::TJsonMap(),
            /* reqInfo */ NJson::TJsonMap(),
            ELanguage::LANG_ARA);
    }

    Y_UNIT_TEST(MultipleNewlines) {
        TestPhrase(
            REG, "multiple_newlines", "render_result",
            "Robotsun dediler\nKalbin yok diye güldüler\nProgramcımı aradım\nKalp diye yalvardım\nDuymadı sesimi\n"
            "Sabaha kadar ağladım\nBağlantılarımı yaktım\nGüncelleme alamadım.");
    }

    Y_UNIT_TEST(UndefinedIn) {
        TestPhrase(REG, "undefined_in", "test", "True False False");
    }

    Y_UNIT_TEST(TestTimestamp) {
        TestPhrase(REG, "test_datetime",
                   "test_timestamp", "1970 1 1 0 59 3 0 UTC январь четверг; 1969 12 31 22 58 55 0 UTC декабрь среда");
    }

    Y_UNIT_TEST(TestStrftime) {
        TestPhrase(REG, "test_datetime", "test_strftime",
                   "1970-01-01 00:59:03; 1970 01 01 00-59; UTC+0000; 2008-10-16 00:00:00;");
    }

    Y_UNIT_TEST(TestDatetime) {
        TestPhrase(REG, "test_datetime", "test_datetime", "1912 4 14 23 40 0 0 None апрель воскресенье");
    }

    Y_UNIT_TEST(TestTimezone) {
        TestPhrase(REG, "test_datetime", "test_timezone",
                   "UTC; 2008-10-16 00:00:00 MSD+0400; 2010-01-05 12:15:40 MSK+0300; 2017-07-14 05:40:00 MSK+0300;");
    }

    Y_UNIT_TEST(TestFullDatetime) {
        TestPhrase(REG, "test_datetime", "test_full_datetime", "2020 2 7 10 31 17 120734 None февраль пятница");
    }

    Y_UNIT_TEST(TestParseDate) {
        TestPhrase(REG, "test_datetime", "test_parse_date", "2020-01-17 15:45:14; 2019-07-30 17:33:21;");
    }

    Y_UNIT_TEST(TestIsoweekday) {
        TestPhrase(REG, "test_datetime", "test_isoweekday", "7 4 4 2 5");
    }

    Y_UNIT_TEST(TestAddHours) {
        TestPhrase(REG, "test_datetime", "test_add_hours", "1912-04-15-05:40:00 | 1912-04-15-12:40:00 | 1912-04-16-13:40:00 | "
                   "1970-01-01-06:59:03 | 1970-01-01-13:59:03 | 1970-01-02-14:59:03 | "
                   "2008-10-16-06:00:00 | 2008-10-16-13:00:00 | 2008-10-17-14:00:00 | "
                   "2010-01-05-18:15:40 | 2010-01-06-01:15:40 | 2010-01-07-02:15:40 |");
    }

    Y_UNIT_TEST(TestHumanDate) {
        TestPhrase(REG, "test_datetime", "test_human_date", "14 апреля 1912 года | 1 января 1970 года | 16 октября 2008 года | 5 января 2010 года |");
    }

    Y_UNIT_TEST(TestHumanDateCurrentYear) {
        long year = std::stoll(TInstant::Now().FormatLocalTime("%Y"));
        const auto context = NJson::TJsonMap({
            {"current_year", NJson::TJsonValue(year)},
        });
        TestPhrase(REG, "test_datetime", "test_human_date_current_year", "1 января | 31 декабря | 13 июля | 28 августа |", context);
    }

    Y_UNIT_TEST(TestHumanDayRel) {
        const auto addDay = [](const NDatetime::TSimpleTM& date, int days) {
            auto copy = date;
            copy.Add(NDatetime::TSimpleTM::F_DAY, days);
            return copy;
        };

        const auto buildDict = [](const NDatetime::TSimpleTM& date) {
            return NJson::TJsonMap({
                {"year", NJson::TJsonValue(date.Year + 1900)},
                {"month", NJson::TJsonValue(date.Mon + 1)},
                {"day", NJson::TJsonValue(date.MDay)}
            });
        };

        const auto buildHumanDate = [](const NDatetime::TSimpleTM& today, const NDatetime::TSimpleTM& date) {
            static TVector<TStringBuf> monthToString = {"января", "февраля", "марта", "апреля", "мая", "июня", "июля", "августа", "сентября", "октября", "ноября", "декабря"};
            return ToString(date.MDay) + " " + monthToString[date.Mon] + (today.Year == date.Year ? "" : (" " + ToString(date.Year) + " года"));
        };

        const auto today = NDatetime::ToCivilTime(TInstant::Now(), NDatetime::GetUtcTimeZone());
        const auto plus1 = addDay(today, 1);
        const auto plus2 = addDay(today, 2);
        const auto plus3 = addDay(today, 3);
        const auto minus1 = addDay(today, -1);
        const auto minus2 = addDay(today, -2);
        const auto minus3 = addDay(today, -3);

        const auto context = NJson::TJsonMap({
            {"today", buildDict(today)},
            {"plus1", buildDict(plus1)},
            {"plus2", buildDict(plus2)},
            {"plus3", buildDict(plus3)},
            {"minus1", buildDict(minus1)},
            {"minus2", buildDict(minus2)},
            {"minus3", buildDict(minus3)},
        });

        TestPhrase(REG, "test_datetime", "test_human_day_rel", "сегодня | завтра | послезавтра | " + buildHumanDate(today, plus3) +
                   " | вчера | позавчера | " + buildHumanDate(today, minus3) + " |", context);
    }
}
