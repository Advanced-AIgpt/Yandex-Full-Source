#pragma once

#include <alice/library/util/rng.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash_set.h>

namespace NBASS::NRadio {

class TRadioRecommender {
public:
    // TODO: use common data class instead of NSc::TValue
    struct TRadioData {
        const TString RadioId; // e.g. "fm_detskoe"
        const TString Title; // e.g. "Дети ФМ"
    };

    struct TWeightedRadioData {
        const TString RadioId;
        const uint32_t Weight;
    };

    using TWeightedModel = THashMap<TString, TVector<TWeightedRadioData>>; // <key, List<station>> (key is metatag/trackId)

public:
    TRadioRecommender(NAlice::IRng& rng, const TVector<TRadioData>& availableRadioData);
    TRadioRecommender& SetDesiredRadioIds(const THashSet<TStringBuf>& desiredRadioIds);
    TRadioRecommender& UseTrackIdWeightedModel(const TStringBuf trackId);
    TRadioRecommender& UseMetatagWeightedModel(const TStringBuf metatag);

    // always returns a reference because "availableRadioData" is not empty
    const TRadioData& Recommend() const;

private:
    NAlice::IRng& Rng_;
    const TVector<TRadioData>& AvailableRadioDatas_;
    const THashSet<TStringBuf>* DesiredRadioIds_ = nullptr;
    TMaybe<TStringBuf> TrackId_ = Nothing(); // use "tracks_to_fm_radio.json" if not Nothing()
    TMaybe<TStringBuf> Metatag_ = Nothing(); // use "metatags_to_fm_radio.json" if not Nothing()

    static const TWeightedModel TrackIdWeightedModel_;
    static const TWeightedModel MetatagWeightedModel_;
};

} // namespace NBASS::NRadio
