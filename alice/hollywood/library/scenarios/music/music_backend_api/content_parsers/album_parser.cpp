#include "album_parser.h"
#include "common.h"
#include "track_parser.h"

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf VOLUMES = "volumes";

void ParseVolume(const NJson::TJsonArray::TArray& tracks, TMusicQueueWrapper& mq, const TMusicContext& mCtx,
                 size_t startOffset, size_t& currentOffset, bool forceChildSafe) {
    const bool hasMusicSubscription = mCtx.GetAccountStatus().GetHasMusicSubscription();
    for (const auto& trackJson : tracks) {
        TParseTrackParams params{
            .Position = currentOffset,
            .AlbumId = mq.ContentId().GetId(),
            .ForceChildSafe = forceChildSafe,
        };
        auto track = ParseTrack(trackJson, params);
        const bool itemAdded = mq.TryAddItem(std::move(track), hasMusicSubscription);
        if (itemAdded && currentOffset < startOffset) {
            mq.MoveTrackFromQueueToHistory();
        }
        ++currentOffset;
    }
}

}

void ParseAlbum(const NJson::TJsonValue& albumJson, TMusicQueueWrapper& mq, TMusicContext& mCtx) {
    const size_t startOffset = mq.GetTrackOffsetIndex();
    size_t currentOffset = mq.GetPagedFirstTrackOffsetIndex(mCtx);

    bool forceChildSafe = false;
    if (albumJson[CHILD_CONTENT].GetBoolean()) {
        forceChildSafe = true;
    }

    if (auto contentInfo = TryConstructContentInfo(albumJson)) {
        mq.SetContentInfo(*contentInfo);
    }

    if (mq.GetShuffle()) {
        if (!albumJson.Has(TRACKS) || !albumJson[TRACKS].IsArray()) {
            return;
        }
        ParseVolume(albumJson[TRACKS].GetArraySafe(), mq, mCtx, startOffset, currentOffset, forceChildSafe);
    } else {
        const auto& volumesJson = albumJson[VOLUMES];
        if (!volumesJson.IsArray()) {
            return;
        }
        for (const auto& volumeJson : volumesJson.GetArraySafe()) {
            ParseVolume(volumeJson.GetArraySafe(), mq, mCtx, startOffset, currentOffset, forceChildSafe);
        }
    }

    if (auto total = albumJson.GetValueByPath(TOTAL_PATH)) {
        mq.SetNextTotalTracks(total->GetIntegerSafe());
    }
    mq.SetTrackOffsetIndex(0);

    if (mCtx.GetFindTrackIdxRequest().GetShouldUseResumeFrom() && albumJson.Has("resumeFrom")) {
        mCtx.SetOffsetMs(albumJson["resumeFrom"]["startPositionSec"].GetInteger() * 1000);
        mCtx.SetUsedSavedProgress(true);
    }
}

} // namespace NAlice::NHollywood::NMusic
