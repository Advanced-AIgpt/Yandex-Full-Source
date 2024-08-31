#pragma once

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/protos/data/video/video.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>

namespace NAlice::NVideoCommon {

TMaybe<NAlice::TWatchedVideoItem> FindVideoInLastWatched(const NAlice::TVideoItem& videoItem,
                                                         const NAlice::TDeviceState& deviceState);

TMaybe<TWatchedTvShowItem> FindTvShowInLastWatched(const NAlice::TVideoItem& videoItem,
                                                   const NAlice::TDeviceState& deviceState);

double CalculateStartAt(double videoLength, double played = 0);

TString BuildResizedThumbnailUri(TStringBuf thumbnail, TStringBuf size = {});
TString BuildThumbnailUri(TStringBuf thumbnail, TStringBuf vhNamespaceSize = {});

} // namespace NAlice::NVideoCommon
