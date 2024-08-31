#include "is_alice_worldwide_language.h"

namespace NAlice::NMegamind {

bool IsAliceWorldWideLanguage(const ELanguage language) {
    return language != ELanguage::LANG_RUS && language != ELanguage::LANG_TUR;
}

ELanguage ConvertAliceWorldWideLanguageToOrdinar(const ELanguage language) {
    return IsAliceWorldWideLanguage(language) ? ELanguage::LANG_RUS : language;
}

}
