#pragma once

#include <library/cpp/langs/langs.h>

namespace NAlice::NMegamind {

// Alice world-wide languages are partially supported and enabled under experiment
bool IsAliceWorldWideLanguage(const ELanguage language);

ELanguage ConvertAliceWorldWideLanguageToOrdinar(const ELanguage language);

}
