#pragma once

#include <util/generic/flags.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/types.h>
#include <functional>

namespace NAlice::NHollywood::NMusic {

enum class EAudioCodec {
    //Values must be bitmask compatible and sorted by quality (higher is better)
    MP3 = 1 /* "mp3" */,
    AAC = 2 /* "aac" */
};

enum class EAudioContainer {
    RAW = 1 /* "raw" */,
    HLS = 2 /* "hls" */
};

enum class DownloadInfoFlag {
    HQ = 1 /* "hq" */,
    LQ = 2 /* "lq" */,
};

struct TDownloadInfoItem {
    EAudioCodec Codec;
    i32 BitrateInKbps;
    bool Gain;
    bool Preview;
    TString DownloadInfoUrl;
    EAudioContainer Container;
    ui64 ExpiringAtMs;
};

Y_DECLARE_FLAGS(TAudioCodecFlags, EAudioCodec);

using TDownloadInfoOptions = TVector<TDownloadInfoItem>;

} // namespace NAlice::NHollywood::NMusic
