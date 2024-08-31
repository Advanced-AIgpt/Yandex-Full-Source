#include "track_quality_selector.h"

#include <library/cpp/iterator/filtering.h>
#include <util/generic/cast.h>

namespace NAlice::NHollywood::NMusic {

namespace {

auto MakeHqCmpTuple(const TDownloadInfoItem& item) {
    //Compare bitrate then codec (aac better than mp3), then gain.
    //Track without replay gain have higher quality than the one with replay gain.
    return std::make_tuple(item.BitrateInKbps, item.Codec, !item.Gain);
}

template<typename TRange, typename TComparator>
const TDownloadInfoItem* TrySelect(const TRange& range, const TComparator& cmp) {
    auto best = MaxElement(range.begin(), range.end(), cmp);
    return best == range.end() ? nullptr : &*best;
}

} // namespace

const TDownloadInfoItem*
TBaseQualitySelector::GenericSelect(const TDownloadInfoOptions& options, const TComparator& cmp) const {
    const auto commonFilter = [this](const auto& item) {
        return IsCodecAllowed(item.Codec) && (!item.Preview || AllowPreview_) && item.Container == AllowedContainer_;
    };

    if (DesiredBitrateInKbps_ != HIGHEST_BITRATE_IN_KBPS) {
        const auto bitrateFilter = [commonFilter, bitrate = DesiredBitrateInKbps_](const auto& item) {
            // try find strongly desired bitrate
            return commonFilter(item) && item.BitrateInKbps == bitrate;
        };
        if (const auto* item = TrySelect(MakeFilteringRange(options, bitrateFilter), cmp)) {
            return item;
        }
    }

    return TrySelect(MakeFilteringRange(options, commonFilter), cmp);
}

const TDownloadInfoItem* THighQualitySelector::operator()(const TDownloadInfoOptions& options) const {
    return GenericSelect(options, [](const auto& a, const auto& b) {
        return MakeHqCmpTuple(a) < MakeHqCmpTuple(b);
    });
}

} // namespace NAlice::NHollywood::NMusic
