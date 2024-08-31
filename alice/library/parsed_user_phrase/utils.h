#pragma once

#include <quality/trailer/trailer_common/normalize.h>

#include <kernel/yawklib/wtrutil.h>

#include <util/charset/wide.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <utility>

class TYandexLemma;

namespace NParsedUserPhrase {

enum EType { MICRO = 1, LEMMA = 2, WORD = 4, EXACT = 8 };

struct TLemma {
    explicit TLemma(const TYandexLemma& lemma);

    static TMaybe<TLemma> FromWord(const TUtf16String& word);
    static TMaybe<TLemma> FromWord(TWtringBuf word);

    TUtf16String Text;
    bool Micro = false;
};

float TypeWeight(EType type);

template <typename TFn>
void ForEachToken(TUtf16String sequence, TUtf16String* normalized, TFn&& fn) {
    sequence.to_lower();

    TUtf16String buffer;
    if (!MakeStrongNormalizedQuery(sequence, &buffer)) {
        if (normalized)
            normalized->clear();
        return;
    }

    if (normalized)
        *normalized = buffer;

    TVector<TWtringBuf> words;
    Wsplit(buffer.begin(), ' ', &words);
    for (const auto& word : words) {
        if (!word.empty())
            fn(word);
    }
}

template <typename TFn>
void ForEachToken(const TString& sequence, TFn&& fn) {
    return ForEachToken(UTF8ToWide(sequence), nullptr /* normalized */,
                        [&fn](TWtringBuf word) { fn(WideToUTF8(word)); });
}

float ComputeWordWeight(const TUtf16String& word);

} // namespace NParsedUserPhrase
