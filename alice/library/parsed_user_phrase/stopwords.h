#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NParsedUserPhrase {

class TStopWordsHolder {
public:
    static constexpr float INVALID_WEIGHT = -1;

public:
    void Add(TStringBuf word, float weight, bool addLemma = true);
    void Add(TUtf16String word, float weight, bool addLemma = true);

    bool Has(const TUtf16String& word) const;

    float GetWeight(const TUtf16String& word) const;

private:
    void AddImpl(const TUtf16String& word, float weight);

    THashMap<TUtf16String, float> Words;
};

const TStopWordsHolder NO_STOP_WORDS;

} // namespace NParsedUserPhrase
