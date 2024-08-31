#include "utils.h"

#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/core/language.h>

#include <util/system/yassert.h>

namespace NParsedUserPhrase {

namespace {

bool IsMicro(const TYandexLemma& lemma) {
    for (const char* c = lemma.GetStemGram(); *c; ++c) {
        EGrammar g = (EGrammar)(unsigned char)(*c);
        if (g == gVerb || g == gSubstantive || g == gAdjective || g == gAdverb || g == gNumeral || g == gAdjNumeral ||
            g == gAdjPronoun || g == gAdvPronoun || g == gSubstPronoun) {
            return false;
        }
    }
    return true;
}

TMaybe<TLemma> FromWordImpl(TWtringBuf word) {
    Y_ASSERT(ToLowerRet(word) == word);

    TWLemmaArray lemmas;
    NLemmer::AnalyzeWord(word.data(), word.size(), lemmas, LANG_RUS);
    if (lemmas.empty())
        return Nothing();

    return TLemma(lemmas[0]);
}

EType DeriveWordType(const TUtf16String& word) {
    const TMaybe<NParsedUserPhrase::TLemma> lemma = NParsedUserPhrase::TLemma::FromWord(word);
    return lemma.Defined() && lemma->Micro ? EType::MICRO : EType::WORD;
}

} // namespace

// TLemma ----------------------------------------------------------------------
TLemma::TLemma(const TYandexLemma& lemma)
    : Text(lemma.GetText(), lemma.GetTextLength())
    , Micro(IsMicro(lemma)) {
}

// static
TMaybe<TLemma> TLemma::FromWord(const TUtf16String& word) {
    return FromWordImpl(ToLowerRet(word));
}

// static
TMaybe<TLemma> TLemma::FromWord(TWtringBuf word) {
    return FromWordImpl(ToLowerRet(word));
}

// -----------------------------------------------------------------------------
float TypeWeight(EType type) {
    switch (type) {
        case MICRO:
            return 0.1;
        case LEMMA:
            return 0.5;
        case WORD:
            return 1.0;
        case EXACT:
            return 1.0;
    }

    Y_ASSERT(false);
    return 0;
}

float ComputeWordWeight(const TUtf16String& word) {
    return TypeWeight(DeriveWordType(word));
}

} // namespace NParsedUserPhrase
