#pragma once

#include "download_info.h"

#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

using TTrackQualitySelector = std::function<const TDownloadInfoItem*(const TDownloadInfoOptions&)>;

class TBaseQualitySelector {
public:
    void SetAllowPreview(bool value) {
        AllowPreview_ = value;
    }

    void SetAllowedCodecs(TAudioCodecFlags value) {
        AllowedCodecs_ = value;
    }

    void SetDesiredBitrateInKbps(const i32 value) {
        DesiredBitrateInKbps_ = value;
    }

    void SetAllowedContainer(EAudioContainer container) {
        AllowedContainer_ = container;
    }

protected:
    using TComparator = std::function<bool(const TDownloadInfoItem& a,
                                           const TDownloadInfoItem& b)>;
protected:
    [[nodiscard]] bool IsCodecAllowed(EAudioCodec codec) const {
        return AllowedCodecs_ & codec;
    }

    [[nodiscard]] const TDownloadInfoItem*
    GenericSelect(const TDownloadInfoOptions& options, const TComparator& cmp) const;

protected:
    static constexpr i32 HIGHEST_BITRATE_IN_KBPS = 320;

    bool AllowPreview_ = false;
    TAudioCodecFlags AllowedCodecs_ = TAudioCodecFlags(EAudioCodec::AAC) | EAudioCodec::MP3;
    i32 DesiredBitrateInKbps_ = HIGHEST_BITRATE_IN_KBPS;
    EAudioContainer AllowedContainer_ = EAudioContainer::RAW;
};

class THighQualitySelector : public TBaseQualitySelector {
public:
    [[nodiscard]] const TDownloadInfoItem* operator()(const TDownloadInfoOptions& options) const;
};

} // namespace NAlice::NHollywood::NMusic
