#pragma once

#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <library/cpp/langs/langs.h>

namespace NAlice {
namespace NItemSelector {

TVector<TString> GetTexts(const TVector<TPhrase>& phrases);

TVector<TPhrase> FilterByLanguage(const TVector<TPhrase>& phrases, const ELanguage language);

TSelectionItem FilterByLanguage(const TSelectionItem& item, const ELanguage language);

TPhrase Lowercase(const TPhrase& phrase);

TVector<TPhrase> Lowercase(const TVector<TPhrase>& phrases);

TSelectionItem Lowercase(const TSelectionItem& item);

TNonsenseToken Lowercase(const TNonsenseToken& token);

TNonsenseTagging Lowercase(const TNonsenseTagging& tagging);

TSelectionRequest Lowercase(const TSelectionRequest& request);

} // namespace NItemSelector
} // namespace NAlice
