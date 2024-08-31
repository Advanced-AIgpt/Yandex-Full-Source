#include "video_helper.h"

#include <alice/library/video_common/defs.h>

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace {

const ui8 CONTINUE_WATCHING_THRESHOLD_SEC = 10; // Do not start later than 10 second to end
const ui8 CONTINUE_WATCHING_GAP_SEC = 5;        // Replay last 5 second again

bool WatchedItemMatch(const NAlice::TVideoItem& videoItem, const NAlice::TWatchedVideoItem& watchedItem) {
    return videoItem.GetProviderItemId() == watchedItem.GetProviderItemId();
}

} // namespace

namespace NAlice::NVideoCommon {

TMaybe<NAlice::TWatchedVideoItem> FindVideoInLastWatched(const NAlice::TVideoItem& videoItem,
                                                         const NAlice::TDeviceState& deviceState) {
    if (!deviceState.HasLastWatched()) {
        return Nothing();
    }

    if (videoItem.GetType() == ToString(NAlice::NVideoCommon::EContentType::Movie)) {
        for (const auto& watchedItem : deviceState.GetLastWatched().GetRawMovies()) {
            if (WatchedItemMatch(videoItem, watchedItem)) {
                return watchedItem;
            }
        }
    }
    if (videoItem.GetType() == ToString(NAlice::NVideoCommon::EContentType::Video)) {
        for (const auto& watchedItem : deviceState.GetLastWatched().GetRawVideos()) {
            if (WatchedItemMatch(videoItem, watchedItem)) {
                return watchedItem;
            }
        }
    }
    if (videoItem.GetType() == ToString(NAlice::NVideoCommon::EContentType::TvShowEpisode)) {
        for (const auto& watchedItem : deviceState.GetLastWatched().GetRawTvShows()) {
            if (WatchedItemMatch(videoItem, watchedItem.GetItem())) {
                return watchedItem.GetItem();
            }
        }
    }
    return Nothing();
}

TMaybe<TWatchedTvShowItem> FindTvShowInLastWatched(const NAlice::TVideoItem& videoItem,
                                                   const NAlice::TDeviceState& deviceState) {
    if (!deviceState.HasLastWatched()) {
        return Nothing();
    }
    for (const auto& watchedItem : deviceState.GetLastWatched().GetRawTvShows()) {
        if (videoItem.GetProviderItemId() == watchedItem.GetTvShowItem().GetProviderItemId()) {
            return watchedItem;
        }
    }
    return Nothing();
}

double CalculateStartAt(double videoLength, double played) {
    if (played == 0) {
        return played;
    }
    double startAt = played - CONTINUE_WATCHING_GAP_SEC;
    if (startAt > videoLength - CONTINUE_WATCHING_THRESHOLD_SEC) {
        startAt = startAt - CONTINUE_WATCHING_GAP_SEC;
    }
    if (startAt < 0) {
        startAt = 0;
    }
    return startAt;
}

TString BuildResizedThumbnailUri(TStringBuf thumbnail, TStringBuf size) {
    TStringBuilder thumbnailUri;
    if (thumbnail.StartsWith("//")) {
        thumbnailUri << "https:";
    } else if (!thumbnail.StartsWith("http")) {
        thumbnailUri << "https://";
    }

    bool needAddSize = !size.empty() && thumbnail.ChopSuffix(TStringBuf("/orig"));
    thumbnailUri << thumbnail;
    if (needAddSize) {
        thumbnailUri << '/' << size;
    }

    return thumbnailUri;
}

TString BuildThumbnailUri(TStringBuf thumbnail, TStringBuf vhNamespaceSize) {
    TStringBuf size;
    // normal avatar from "vh" namespace
    if (thumbnail.Contains(TStringBuf("get-vh"))) {
        size = vhNamespaceSize.Empty() ? TStringBuf("640x360") : vhNamespaceSize;
    } else if (thumbnail.Contains(TStringBuf("get-ott"))) {
        size = TStringBuf("504x284");
    }

    return BuildResizedThumbnailUri(thumbnail, size);
}

} // namespace NAlice::NVideoCommon
