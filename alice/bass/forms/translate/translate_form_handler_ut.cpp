#include <alice/bass/forms/translate/translate.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>
#include <util/string/split.h>

Y_UNIT_TEST_SUITE(TTranslateFormHandlerTest) {
    Y_UNIT_TEST(DetectLangDir) {
        static const struct {
            TUtf16String Text;
            TUtf16String InputLangSrc;
            TUtf16String InputLangDst;
            ELanguage LangSrc;
            ELanguage LangDst;
            TString UnsupportedLangSrc;
            TString UnsupportedLangDst;
            bool Result;
        } tests[] = {
            { u"кошку", u"русского", u"английский", LANG_RUS, LANG_ENG, "", "", true },
            { u"table", u"французского", u"корейский", LANG_FRE, LANG_KOR, "", "", true },
            { u"собака", u"", u"абхазском", LANG_UNK, LANG_UNK, "", "абхазский", false },
            { u"стол", u"", u"", LANG_UNK, LANG_UNK, "", "", true },
            { u"кошку", u"древнерусского", u"древнегреческий", LANG_UNK, LANG_UNK, "древнерусский", "древнегреческий", false },
            { u"кружку", u"русского", u"чеченский", LANG_RUS, LANG_UNK, "", "чеченский", false }
        };

        NBASS::TTranslateFormHandler translateFormHandler;
        translateFormHandler.LoadLanguageStems();
        translateFormHandler.LoadLanguages();

        for (const auto& t : tests) {
            NBASS::TTranslateFormHandler::TSegmentation segmentation;
            TString unsupportedLangSrc = "", unsupportedLangDst = "";
            bool result = (translateFormHandler.ParseQuerySegmentation(t.Text, t.InputLangSrc, t.InputLangDst,
                                                        segmentation, unsupportedLangSrc, unsupportedLangDst) == "");
            UNIT_ASSERT_EQUAL(result, t.Result);
            UNIT_ASSERT_EQUAL(segmentation.From, t.LangSrc);
            UNIT_ASSERT_EQUAL(segmentation.To, t.LangDst);
            UNIT_ASSERT_EQUAL(unsupportedLangSrc, t.UnsupportedLangSrc);
            UNIT_ASSERT_EQUAL(unsupportedLangDst, t.UnsupportedLangDst);
        }
    }
}
