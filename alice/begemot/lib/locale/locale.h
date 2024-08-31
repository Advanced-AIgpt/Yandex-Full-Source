#pragma once

#include <kernel/relev_locale/serptld.h>
#include <search/begemot/rules/internal/locale/proto/locale.pb.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/langs/langs.h>

namespace NAlice::NAliceLocale {

inline const TLangMask SUPPORTED_LANGUAGES = {LANG_RUS, LANG_ENG, LANG_TUR, LANG_KAZ};

inline ELanguage TldToLanguage(EYandexSerpTld tld, ELanguage def = LANG_UNK) {
    if (tld == YST_RU) {
        return LANG_RUS;
    } else if (tld == YST_TR) {
        return LANG_TUR;
    } else if (tld == YST_COM) {
        return LANG_ENG;
    } else if (tld == YST_KZ) {
        return LANG_KAZ;
    } else {
        return def;
    }
}

inline EYandexSerpTld LanguageToTld(ELanguage lang, EYandexSerpTld def = YST_UNKNOWN) {
    if (lang == LANG_RUS) {
        return YST_RU;
    } else if (lang == LANG_TUR) {
        return YST_TR;
    } else if (lang == LANG_ENG) {
        return YST_COM;
    } else if (lang == LANG_ARA) {
        return YST_COM;
    } else if (lang == LANG_KAZ) {
        return YST_KZ;
    } else {
        return def;
    }
}

inline ELanguage GetLanguageRobust(const NBg::NProto::TLocaleResult& locale) {
    const ui32 lang = locale.GetLanguage();
    if (lang != LANG_UNK && lang < LANG_MAX) {
        return static_cast<ELanguage>(lang);
    }
    return TldToLanguage(locale.GetTld(), LANG_RUS);
}

// FYI:
// Useful functions from library/cpp/langs/langs.h:
//   IsoNameByLanguage - ELanguage to short name (ru, tr, kk).
//   LanguageByName - name to ELanguage.

} // namespace NAlice::NAliceLocale
