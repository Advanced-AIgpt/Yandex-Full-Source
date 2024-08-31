#include "request_tokenizer.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NNlu;

Y_UNIT_TEST_SUITE(TRequestTokenizer) {

    void Test(const TString& text, const TString& expected) {
        const TString actual = JoinSeq(" ", TRequestTokenizer::Tokenize(text));
        if (expected == actual) {
            return;
        }
        TStringBuilder message;
        message << Endl;
        message << "TRequestTokenizer error:" << Endl;
        message << "  text:     " << text << Endl;
        message << "  expected: " << expected << Endl;
        message << "  actual:   " << actual << Endl;
        UNIT_FAIL(message);
    }

    Y_UNIT_TEST(Numbers) {
        Test("2",                               "2");
        Test("2-й",                             "2 - й");
        Test("2'й",                             "2 й");
        Test("2й",                              "2 й");
        Test("2ой",                             "2 ой");
        Test("2-ой",                            "2 - ой");
        Test("100 11 2",                        "100 11 2");
        Test("100 11-й 2",                      "100 11 - й 2");
        Test("СТО-11 2.1",                      "СТО - 11 2.1");
        Test("20:00",                           "20:00");
        Test("20:05",                           "20:05");
        Test("+7 495 500 50 03",                "+ 7 495 500 50 03");
        Test("+7(495)500-50-03",                "+ 7 495 500 - 50 - 03"); // bad
        Test("Первый, второй; третий. Четвёртый?", "Первый второй третий Четвёртый");
        Test("полтораста",                      "полтораста");
        Test("1, 2, 3, 4.",                     "1 2 3 4");
        Test("1,2,3,4.",                        "1,2 3,4");
        Test("1\t2\n3 4",                       "1 2 3 4");
        Test("8.5, 3.1. 7,1.",                  "8.5 3.1 7,1");
        Test("-8.5, -3.1. -7,1.",               "-8.5 -3.1 -7,1");
        Test("-8.5,-3.1.-7,1.",                 "-8.5 -3.1 -7,1");
        Test("Николай II",                      "Николай II");
        Test("2 + 2 = 4",                       "2 + 2 = 4");
        Test("2+2 = 4",                         "2 + 2 = 4");
        Test("-3 - 2 = -5",                     "-3 - 2 = -5");
        Test("(-2.3 + 2.01) * 4 / 2 = 0",       "-2.3 + 2.01 * 4 / 2 = 0");
        Test("На улице -5",                     "На улице -5");
        Test("На улице +5",                     "На улице + 5");
        Test("На улице - 5",                    "На улице - 5");
        Test("На улице + 5",                    "На улице + 5");
    }

    Y_UNIT_TEST(WordWithWord) {
        Test("Три-четыре.",                     "Три-четыре");
        Test("Ростов-на-Дону",                  "Ростов-на-Дону");
        Test("Включи-ка что-нибудь (Дом-2, например).", "Включи-ка что-нибудь Дом - 2 например");
        Test("α-частица?",                      "α-частица");
        Test("Don't",                           "Don t");
        Test("поллитра",                        "поллитра");
        Test("пол-литра",                       "пол-литра");
        Test("полседьмого",                     "полседьмого");
        Test("шеститысячный",                   "шеститысячный");
        Test("двадцатипятитысячный",            "двадцатипятитысячный");
        Test("стосорокашестисотмиллионный",     "стосорокашестисотмиллионный");
        Test("тридцатиодномиллиардный",         "тридцатиодномиллиардный");
    }

    Y_UNIT_TEST(WordWithNumbers) {
        Test("АИ95",                            "АИ 95");
        Test("дом 8/2",                         "дом 8 / 2");
        Test("дом 8/а",                         "дом 8 / а");
        Test("5 м/с",                           "5 м/с");
        Test("1А класс",                        "1 А класс");
        Test("5 р",                             "5 р");
        Test("$5",                              "$ 5");
    }

    Y_UNIT_TEST(Diacritics) {
        Test("четвёртый",                       "четвёртый");
        Test("четвертый",                       "четвертый");
        Test("всё",                             "всё");
    }

    Y_UNIT_TEST(Accent) {
        Test("со́рок - имя числительное.",       "со рок - имя числительное");
        Test("сорок - имя числительное.",       "сорок - имя числительное");
    }
}
