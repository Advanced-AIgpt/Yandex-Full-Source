#include "is_alice_worldwide_language.h"
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

Y_UNIT_TEST_SUITE(AliceWorldWideLanguage) {

    Y_UNIT_TEST(TestIsAliceWorldWideLanguage) {
        UNIT_ASSERT_EQUAL(IsAliceWorldWideLanguage(ELanguage::LANG_RUS), false);
        UNIT_ASSERT_EQUAL(IsAliceWorldWideLanguage(ELanguage::LANG_TUR), false);

        UNIT_ASSERT_EQUAL(IsAliceWorldWideLanguage(ELanguage::LANG_ARA), true);
        UNIT_ASSERT_EQUAL(IsAliceWorldWideLanguage(ELanguage::LANG_ENG), true);
    }

    Y_UNIT_TEST(TestConvertAliceWorldWideLanguageToOrdinar) {
        UNIT_ASSERT_EQUAL(ConvertAliceWorldWideLanguageToOrdinar(ELanguage::LANG_RUS), ELanguage::LANG_RUS);
        UNIT_ASSERT_EQUAL(ConvertAliceWorldWideLanguageToOrdinar(ELanguage::LANG_TUR), ELanguage::LANG_TUR);

        UNIT_ASSERT_EQUAL(ConvertAliceWorldWideLanguageToOrdinar(ELanguage::LANG_ARA), ELanguage::LANG_RUS);
        UNIT_ASSERT_EQUAL(ConvertAliceWorldWideLanguageToOrdinar(ELanguage::LANG_ENG), ELanguage::LANG_RUS);
    }

}

} // NAlice::NMegamind
