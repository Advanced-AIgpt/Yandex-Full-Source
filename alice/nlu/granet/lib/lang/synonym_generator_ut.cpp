#include "synonym_generator.h"
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

namespace NGranet {

Y_UNIT_TEST_SUITE(GenerateBuiltinSynonyms) {

    void Test(TStringBuf word, TStringBuf expected) {
        TStringBuilder actual;
        for (const TStringWithWeight& variant : GenerateBuiltinSynonyms(word)) {
            actual << variant.String << '\n';

        }
        NAlice::NUtUtils::TestEqual(word, expected, actual);
    }

    Y_UNIT_TEST(VerbGood) {
        Test("включи", R"(
            включай
            включайте
            включи
            включите
            включить
            можешь включить
        )");
        Test("запусти", R"(
            запусти
            запустите
            запустить
            можешь запустить
        )");
        Test("запускай", R"(
            запускай
            запускайте
            запускать
        )");
        Test("повтори", R"(
            можешь повторить
            повтори
            повторите
            повторить
            повторяй
            повторяйте
        )");
        Test("проиграй", R"(
            можешь проиграть
            проиграй
            проиграйте
            проиграть
            проигрывай
            проигрывайте
        )");
        Test("пой", R"(
            петь
            пой
            пойте
        )");
        Test("спой", R"(
            можешь спеть
            спеть
            спой
            спойте
        )");
        Test("играй", R"(
            играй
            играйте
            играть
        )");
        Test("вруби", R"(
            врубай
            врубайте
            вруби
            врубите
            врубить
            можешь врубить
        )");
        Test("врубай", R"(
            врубай
            врубайте
            врубать
        )");
    }

    Y_UNIT_TEST(LetsVsGive) {
        Test("дай", R"(
            давай
            давайте
            дай
            дайте
            дать
            можешь дать
        )");
        Test("давай", R"(
            давай
            давайте
            давать
        )");
    }

    Y_UNIT_TEST(VerbNotGood) {
        Test("запусти", R"(
            запусти
            запустите
            запустить
            можешь запустить
        )");
        Test("запускай", R"(
            запускай
            запускайте
            запускать
        )");
        Test("поставь", R"(
            можешь поставить
            поставить
            поставляй
            поставляйте
            поставь
            поставьте
        )");
        Test("ставь", R"(
            ставить
            ставь
            ставьте
        )");
        Test("спой", R"(
            можешь спеть
            спеть
            спой
            спойте
        )");
        Test("пой", R"(
            петь
            пой
            пойте
        )");
        Test("играй", R"(
            играй
            играйте
            играть
        )");
    }
}

} // namespace NGranet
