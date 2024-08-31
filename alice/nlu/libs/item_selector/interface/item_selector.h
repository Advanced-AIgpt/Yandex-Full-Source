#pragma once

#include <library/cpp/langs/langs.h>

#include <util/charset/utf8.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>

namespace NAlice {
namespace NItemSelector {

struct TPhrase {
    TString Text;
    ELanguage Language = ELanguage::LANG_UNK;

    bool operator==(const TPhrase& other) const {
        return Language == other.Language && Text == other.Text;
    }
};

struct TSelectionResult {
    double Score = -1;
    bool IsSelected = false;
};

struct TNonsenseToken {
    TString Text;
    double NonsenseProbability = -1;
};

using TNonsenseTagging = TVector<TNonsenseToken>;
using TEntityType = TString;
using TSelectorName = TString;

struct TSelectionRequest {
    TPhrase Phrase;
    TNonsenseTagging NonsenseTagging;
    TMaybe<TEntityType> EntityType = Nothing();
    TMaybe<TSelectorName> SelectorName = Nothing();
};

struct TVideoItemMeta {
    uint32_t Duration = 0;
    uint32_t Episode = 0;
    uint32_t EpisodesCount = 0;
    TString Genre;
    uint32_t MinAge = 0;
    TString ProviderName;
    double Rating = 0;
    uint32_t ReleaseYear = 0;
    uint32_t Season = 0;
    uint32_t SeasonsCount = 0;
    TString Type;
    uint64_t ViewCount = 0;
    uint32_t Position = 0;
};

struct TSelectionItem {
    TVector<TPhrase> Synonyms;
    TVector<TPhrase> Negatives;
    std::variant<std::monostate, TVideoItemMeta> Meta;
};

class IItemSelector {
public:
    virtual ~IItemSelector() = default;
    virtual TVector<TSelectionResult> Select(const TSelectionRequest& request,
                                             const TVector<TSelectionItem>& items) const = 0;
};

} // namespace NItemSelector
} // namespace NAlice
