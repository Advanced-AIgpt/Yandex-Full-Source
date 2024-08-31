#include "normalization.h"

#include <alice/begemot/lib/locale/locale.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NBg {

    void NormalizePhrase(const TStringBuf phrase, const NProto::TLocaleResult& locale,
                         TString* normalizedPhrase, TVector<TString>* normalizedTokens) {
        const ELanguage language = NAlice::NAliceLocale::GetLanguageRobust(locale);

        const auto normalizerOutput = NNlu::TRequestNormalizer::Normalize(language, phrase);

        if (normalizedPhrase) {
            *normalizedPhrase = normalizerOutput;
        }

        if (normalizedTokens) {
            *normalizedTokens = StringSplitter(normalizerOutput).Split(' ').ToList<TString>();
        }
    }

} // namespace NBg
