#pragma once

#include <alice/library/logger/logger.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/hash.h>

namespace NAlice::NHollywood::NWeather {

class TTranslations {
public:
    TTranslations(TRTLogger& logger, const ELanguage language);
    TString Translate(const TString& key) const;

private:
    TRTLogger& Logger_;
    const THashMap<TString, TString>& TranslationsMap_;
};

} // namespace NAlice::NHollywood::NWeather
