#include "common.h"

#include <util/charset/utf8.h>

namespace NAlice {
namespace NItemSelector {

TVector<TString> GetTexts(const TVector<TPhrase>& phrases) {
    TVector<TString> texts;
    for (const TPhrase& phrase : phrases) {
        texts.push_back(phrase.Text);
    }
    return texts;
}

TVector<TPhrase> FilterByLanguage(const TVector<TPhrase>& phrases, const ELanguage language) {
    TVector<TPhrase> filtered;
    for (const TPhrase& phrase : phrases) {
        if (phrase.Language == language) {
            filtered.push_back(phrase);
        }
    }
    return filtered;
}

TSelectionItem FilterByLanguage(const TSelectionItem& item, const ELanguage language) {
    TSelectionItem filtered = item;
    filtered.Synonyms = FilterByLanguage(item.Synonyms, language);
    filtered.Negatives = FilterByLanguage(item.Negatives, language);
    return filtered;
}

TPhrase Lowercase(const TPhrase& phrase) {
    return {
        .Text = ToLowerUTF8(phrase.Text),
        .Language = phrase.Language
    };
}

TVector<TPhrase> Lowercase(const TVector<TPhrase>& phrases) {
    TVector<TPhrase> lowercased;
    for (const TPhrase& phrase : phrases) {
        lowercased.push_back(Lowercase(phrase));
    }
    return lowercased;
}

TSelectionItem Lowercase(const TSelectionItem& item) {
    TSelectionItem lowercased = item;
    lowercased.Synonyms = Lowercase(item.Synonyms);
    lowercased.Negatives = Lowercase(item.Negatives);
    return lowercased;
}

TNonsenseToken Lowercase(const TNonsenseToken& token) {
    return {
        .Text = ToLowerUTF8(token.Text),
        .NonsenseProbability = token.NonsenseProbability
    };
}

TNonsenseTagging Lowercase(const TNonsenseTagging& tagging) {
    TNonsenseTagging lowercased;
    for (const TNonsenseToken& token : tagging) {
        lowercased.push_back(Lowercase(token));
    }
    return lowercased;
}

TSelectionRequest Lowercase(const TSelectionRequest& request) {
    TSelectionRequest lowercased = request;
    lowercased.Phrase = Lowercase(request.Phrase);
    lowercased.NonsenseTagging = Lowercase(request.NonsenseTagging);
    return lowercased;
}

} // namespace NItemSelector
} // namespace NAlice
