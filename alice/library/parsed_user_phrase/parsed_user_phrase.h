#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NParsedUserPhrase {

class TParsedUserPhrase {
public:
    static const float INVALID_SCORE;

public:
    void Parse(TUtf16String userCommand);
    bool Empty() const;
    float Match(const TParsedUserPhrase& etalon) const;

private:
    int Types(TUtf16String w) const;

private:
    THashMap<TUtf16String, int> Data;
};

bool MatchPhrases(const TParsedUserPhrase& userPhrase, const TParsedUserPhrase& predefinedPhrase,
                  float userMatchThreshold = 0.5, float predefinedMatchThreshold = 0.11);

} // namespace NParsedUserPhrase
