#include <alice/nlg/library/runtime_api/exceptions.h>
#include <alice/nlg/library/runtime_api/postprocess.h>
#include <alice/nlg/library/runtime_api/text_stream.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/escape.h>

using namespace NAlice::NNlg;
using namespace NAlice::NNlg::NPrivate;

Y_UNIT_TEST_SUITE(NlgPostprocessing) {
    Y_UNIT_TEST(SpecialPunct) {
        TStringBuf positiveCases[] = {".", ",", ":", "!", "?", ";", "»"};
        for (TStringBuf str : positiveCases) {
            UNIT_ASSERT_C(IsSpecialPunct(str), "str = \"" << EscapeC(str) << '"');
        }

        TStringBuf negativeCases[] = {"f", "$", "\"", "ф", " ", ""};
        for (TStringBuf str : negativeCases) {
            UNIT_ASSERT_C(!IsSpecialPunct(str), "str = \"" << EscapeC(str) << '"');
        }
    }

    Y_UNIT_TEST(CollapsePunctWhitespace) {
        std::pair<TStringBuf, TStringBuf> cases[] = {
            {"", ""},
            {"\n", " "},
            {" \t  ", " "},
            {"   a", " a"},
            {"   .", "."},
            {". \n ", ". "},
            {"   .  ,  :  !  ?  ;  »  ", ".,:!?;» "},
            {" asdf. фыва. ", " asdf. фыва. "},
            {" фыва .", " фыва."},
        };

        for (auto [str, expected] : cases) {
            UNIT_ASSERT_VALUES_EQUAL_C(expected, CollapsePunctWhitespace(str),
                                       "str = \"" << EscapeC(str) << '"');
        }
    }

    Y_UNIT_TEST(UnquoteNewlines) {
        std::pair<TStringBuf, TStringBuf> cases[] = {
            {"", ""},
            {" ", " "},
            {"фыва", "фыва"},
            {" \n ", " \n "},
            {"\\n", "\n"},
            {"\t\\n", "\n"},
            {"\\n\r", "\n"},
            {"   \\n   \\n   ", "\n\n"},
            {"  asdf   \\n   jkl;", "  asdf\njkl;"},
            {"  фыва   \\n   олдж", "  фыва\nолдж"},
            {"  фыва   \\n   олдж  ", "  фыва\nолдж  "},
            {"  фыва   \\n   олдж  \\nasdf", "  фыва\nолдж\nasdf"},
            {"фыва\\nолдж\\nasdf", "фыва\nолдж\nasdf"},
            {"фыва\\n олдж\\n asdf", "фыва\nолдж\nasdf"},
            {"фыва \\nолдж \\nasdf", "фыва\nолдж\nasdf"},
        };

        for (auto [str, expected] : cases) {
            UNIT_ASSERT_VALUES_EQUAL_C(expected, UnquoteNewlines(str),
                                       "str = \"" << EscapeC(str) << '"');
        }
    }

    Y_UNIT_TEST(PostprocessPhraseString) {
        {
            TStringBuf str = "Привет, мир ! \\n Неси\n  черешню, »!";
            TStringBuf expected = "Привет, мир!\nНеси черешню,»!";

            UNIT_ASSERT_VALUES_EQUAL(expected, PostprocessPhraseString(str));
        }
        {
            TStringBuf str = "Привет, мир ! \\n Неси\\n  черешню, »!";
            TStringBuf expected = "Привет, мир!\nНеси\nчерешню,»!";

            UNIT_ASSERT_VALUES_EQUAL(expected, PostprocessPhraseString(str));
        }
    }

    Y_UNIT_TEST(PostprocessCardDiv1Positive) {
        TText text{"{\"states\": []}"};

        NJson::TJsonValue expected;
        expected["states"] = NJson::TJsonValue(NJson::JSON_ARRAY);

        UNIT_ASSERT_VALUES_EQUAL(expected, PostprocessCard(text));
    }

    Y_UNIT_TEST(PostprocessCardDiv1ReduceWhitespace) {
        TText text{"{\"states\": {\"x1\": \"    very     long     string                     too long!        \", \"x2\": 13, \"x3\": [ \" aa aa aa \", \"   b,    b    b,   b.  \" ]}}"};

        NJson::TJsonValue expected;

        NJson::TJsonValue map;
        map["x1"] = "very long string too long!";
        map["x2"] = 13;

        NJson::TJsonArray arr;
        arr.AppendValue("aa aa aa");
        arr.AppendValue("b, b b, b.");
        map["x3"] = arr;

        expected["states"] = map;

        UNIT_ASSERT_VALUES_EQUAL(expected, PostprocessCard(text, /* reduceWhitespace = */ true));
    }

    Y_UNIT_TEST(PostprocessCardDiv1Negative) {
        // invalid JSON
        UNIT_ASSERT_EXCEPTION(PostprocessCard(TText{"{\"foo\": 1"}), TCardValidationError);

        // no "states" key
        UNIT_ASSERT_EXCEPTION(PostprocessCard(TText{"{\"foo\": []}"}), TCardValidationError);

        // unexpected "templates" key
        UNIT_ASSERT_EXCEPTION(PostprocessCard(TText{"{\"templates\": {}, \"states\": []}"}), TCardValidationError);
    }

    Y_UNIT_TEST(PostprocessCardDiv2Positive) {
        {
            TText text{"{\"card\": {}}"};

            NJson::TJsonValue expected;
            expected["card"] = NJson::TJsonValue(NJson::JSON_MAP);

            UNIT_ASSERT_VALUES_EQUAL(expected, PostprocessCard(text));
        }

        {
            TText text{"{\"card\": {}, \"templates\": {}}"};

            NJson::TJsonValue expected;
            expected["card"] = NJson::TJsonValue(NJson::JSON_MAP);
            expected["templates"] = NJson::TJsonValue(NJson::JSON_MAP);

            UNIT_ASSERT_VALUES_EQUAL(expected, PostprocessCard(text));
        }
    }

    Y_UNIT_TEST(PostprocessCardDiv2Negative) {
        // invalid JSON
        UNIT_ASSERT_EXCEPTION(PostprocessCard(TText{"{\"foo\": 1"}), TCardValidationError);

        // extra "states" key
        UNIT_ASSERT_EXCEPTION(PostprocessCard(TText{"{\"states\": [], \"card\": {}}"}), TCardValidationError);

        // extra "background" key
        UNIT_ASSERT_EXCEPTION(PostprocessCard(TText{"{\"background\": [], \"card\": {}}"}), TCardValidationError);
    }

    Y_UNIT_TEST(PostprocessPhrase) {
        TText text;
        TTextOutput out(text);

        out << ClearFlag(TText::EFlag::Voice)
            << "Текст !"
            << SetFlag(TText::EFlag::Voice)
            << "   "
            << ClearFlag(TText::EFlag::Text)
            << "Голос!";

        auto actual = PostprocessPhrase(text);
        UNIT_ASSERT_VALUES_EQUAL("Текст!", actual.Text);
        UNIT_ASSERT_VALUES_EQUAL("Голос!", actual.Voice);
    }

    Y_UNIT_TEST(PostprocessBadJsonLog) {
        // Error at key1
        static TString textErrorFront = "{\"key1\":123456], \"key2\":\"long_long_value2\", \"key3\":\"long_long_value3\","
                                        "\"key4\":\"long_long_value4\", \"key5\":\"long_long_value5\", \"key6\":\"long_long_value6\"}";
        // Error at key8
        static TString textErrorMiddle = "{\"key1\":123456, \"key2\":\"long_long_value2\", \"key3\":\"long_long_value3\","
                                         "\"key4\":\"long_long_value4\", \"key5\":\"long_long_value5\", \"key6\":\"long_long_value6\"},"
                                         "\"key7\":\"long_long_value7\", \"key8\":\"long_long_value8\"{}, \"key9\":\"long_long_value9\"},"
                                         "\"key10\":\"long_long_value10\", \"key11\":\"long_long_value11\", \"key12\":\"long_long_value12\"}";
        // Error at key6
        static TString textErrorBack = "{\"key1\":123456, \"key2\":\"long_long_value2\", \"key3\":\"long_long_value3\","
                                       "\"key4\":\"long_long_value4\", \"key5\":\"long_long_value5\", \"key6\":\"long_long_value6}";

        try {
            PostprocessCard(TText{textErrorFront});
        } catch (const yexception& exc) {
            UNIT_ASSERT(exc.AsStrBuf().find("{\"key1\":123456], \"key2\":\"long_long_value2\", \"key3\":\"long_long_value3\",") != TStringBuf::npos);
        }
        try {
            PostprocessCard(TText{textErrorMiddle});
        } catch (const yexception& exc) {
            UNIT_ASSERT(exc.AsStrBuf().find("long_long_value3\",\"key4\":\"long_long_value4\", \"key5\":\"long_long_value5\", \"key6\":\"long_long_value6\"},") != TStringBuf::npos);
        }
        try {
            PostprocessCard(TText{textErrorBack});
        } catch (const yexception& exc) {
            UNIT_ASSERT(exc.AsStrBuf().find("key4\":\"long_long_value4\", \"key5\":\"long_long_value5\", \"key6\":\"long_long_value6}") != TStringBuf::npos);
        }
     }
}
