#pragma once

#include <kernel/lemmer/core/lemmer.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/fwd.h>

namespace NNlu {

    TWLemmaArray GenerateLemmas(TWtringBuf word, ELanguage lang);
    TWLemmaArray GenerateLemmasWithThreshold(TWtringBuf word, ELanguage lang, double threshold);

    const double ANY_LEMMA_THRESHOLD = 0.;
    const double GOOD_LEMMA_THRESHOLD = 0.2;
    const double SURE_LEMMA_THRESHOLD = 1.;

    TVector<TString> LemmatizeWord(TStringBuf word, ELanguage lang, double threshold);
    TVector<TUtf16String> LemmatizeWord(TWtringBuf word, ELanguage lang, double threshold);

    TUtf16String LemmatizeWordBest(TWtringBuf word, ELanguage lang);
    TString LemmatizeWordBest(TStringBuf word, ELanguage lang);

} // namespace NNlu
