#include "stopwords.h"

#include "utils.h"

#include <util/charset/wide.h>

#include <algorithm>

namespace NParsedUserPhrase {

// static
const float TStopWordsHolder::INVALID_WEIGHT;

void TStopWordsHolder::Add(TStringBuf word, float weight, bool addLemma) {
    Add(UTF8ToWide(word), weight, addLemma);
}

void TStopWordsHolder::Add(TUtf16String word, float weight, bool addLemma) {
    word.to_lower();
    AddImpl(word, weight);
    if (addLemma) {
        if (const auto lemma = TLemma::FromWord(word)) {
            AddImpl(lemma->Text, weight);
        }
    }
}

bool TStopWordsHolder::Has(const TUtf16String& word) const {
    return Words.contains(to_lower(word));
}

float TStopWordsHolder::GetWeight(const TUtf16String& word) const {
    const auto it = Words.find(to_lower(word));
    return it == Words.end() ? INVALID_WEIGHT : it->second;
}

void TStopWordsHolder::AddImpl(const TUtf16String& word, float weight) {
    auto it = Words.find(word);
    if (it != Words.end()) {
        it->second = std::max(it->second, weight);
        return;
    }

    Words[word] = weight;
}

} // namespace NParsedUserPhrase
