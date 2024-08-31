#include "token.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/token_aligner/joined_tokens.h>
#include <kernel/lemmer/core/language.h>
#include <library/cpp/token/charfilter.h>
#include <util/string/type.h>

namespace NGranet::NUserEntity {

static bool IsStrong(const TYandexLemma& lemma) {
    for (const char* c = lemma.GetStemGram(); *c; ++c) {
        const EGrammar g = static_cast<EGrammar>(static_cast<unsigned char>(*c));
        if (EqualToOneOf(g, gVerb, gSubstantive, gAdjective, gAdverb, gNumeral, gAdjNumeral,
            gAdjPronoun, gAdvPronoun, gSubstPronoun))
        {
            return true;
        }
    }
    return false;
}

// ~~~~ TTokenBuilder ~~~~

TTokenBuilder::TTokenBuilder(const TLangMask& languages)
    : Languages(languages)
    , Tokenizer(languages)
{
    MakeId(TUtf16String()); // reserve zero id for empty string, just in case
}

TVector<TTokenInfo> TTokenBuilder::Tokenize(TWtringBuf text, TUtf16String* outOriginalTokens) {
    TUtf16String originalTokens = Tokenizer.Tokenize(text);

    TVector<TTokenInfo> tokens;
    TWtringBuf rest = originalTokens;
    while (!rest.empty()) {
        tokens.push_back(MakeToken(rest.NextTok(' ')));
    }

    if (outOriginalTokens) {
        *outOriginalTokens = std::move(originalTokens);
    }
    return tokens;
}

const TTokenInfo& TTokenBuilder::MakeToken(TWtringBuf word) {
    auto [it, isNew] = TokenInfoCache.try_emplace(word);
    TTokenInfo& token = it->second;
    if (!isNew) {
        return token;
    }

    token.Original = MakeId(TUtf16String(word));
    token.Normalized = MakeId(NormalizeUnicode(word));

    if (IsNumber(word)) {
        token.Lemma = token.Normalized;
        token.Flags = TF_STRONG;
        return token;
    }

    TWLemmaArray lemmas;
    NLemmer::AnalyzeWord(word.data(), word.size(), lemmas, Languages);
    if (lemmas.empty()) {
        token.Lemma = token.Normalized;
        return token;
    }

    const TYandexLemma& best = lemmas.front();
    token.Lemma = MakeId(NormalizeUnicode(TWtringBuf(best.GetText(), best.GetTextLength())));
    SetFlags(&token.Flags, TF_STRONG, IsStrong(best));
    return token;
}

ui32 TTokenBuilder::MakeId(TUtf16String&& word) {
    auto [it, isNew] = WordToId.try_emplace(std::move(word), static_cast<ui32>(WordToId.size()));
    if (isNew) {
        Y_ENSURE(IdToWord.size() == it->second);
        IdToWord.push_back(&it->first);
    }
    return it->second;
}

ui32 TTokenBuilder::GetIdCounterValue() const {
    return static_cast<ui32>(WordToId.size());
}

} // namespace NGranet::NUserEntity
