#include "parsed_user_phrase.h"

#include "utils.h"

#include <quality/trailer/trailer_common/normalize.h>

#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>

#include <kernel/yawklib/wtrutil.h>

namespace NParsedUserPhrase {

// static
const float TParsedUserPhrase::INVALID_SCORE = -1;

void TParsedUserPhrase::Parse(TUtf16String userCommand) {
    userCommand.to_lower();
    TUtf16String normalized;
    if (!MakeStrongNormalizedQuery(userCommand, &normalized))
        return;

    Data[normalized] |= EXACT;
    TVector<TWtringBuf> words;
    Wsplit(normalized.begin(), ' ', &words);
    for (TWtringBuf w : words) {
        if (w.empty())
            continue;

        const auto lemma = TLemma::FromWord(w);
        if (!lemma) {
            Data[w] |= WORD;
            Data[w] |= LEMMA;
        } else if (lemma->Micro) {
            Data[w] |= MICRO;
        } else {
            Data[w] |= WORD;
            Data[lemma->Text] |= LEMMA;
        }
    }
}

bool TParsedUserPhrase::Empty() const {
    return Data.empty();
}

float TParsedUserPhrase::Match(const TParsedUserPhrase& etalon) const {
    float total = 0, matched = 0;

    for (auto it : Data) {
        const int types = it.second;
        const int etalonTypes = etalon.Types(it.first);

        if ((types & EXACT) && (etalonTypes & EXACT))
            return 1.0;

        for (const auto type : {WORD, LEMMA, MICRO}) {
            if (types & type) {
                const float weight = TypeWeight(type);
                total += weight;
                if (etalonTypes & type)
                    matched += weight;
            }
        }
    }

    if (matched == 0)
        return INVALID_SCORE;

    return matched / total;
}

int TParsedUserPhrase::Types(TUtf16String w) const {
    const auto it = Data.find(w);
    return it == Data.end() ? 0 : it->second;
}

bool MatchPhrases(const TParsedUserPhrase& userPhrase, const TParsedUserPhrase& predefinedPhrase,
                  float userMatchThreshold, float predefinedMatchThreshold) {
    const bool userMatch = userPhrase.Match(predefinedPhrase) >= userMatchThreshold;
    const bool predefinedMatch = predefinedPhrase.Match(userPhrase) >= predefinedMatchThreshold;

    return userMatch && predefinedMatch;
}

} // namespace NParsedUserPhrase
