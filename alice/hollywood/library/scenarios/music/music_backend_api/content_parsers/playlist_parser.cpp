#include "common.h"
#include "playlist_parser.h"
#include "track_parser.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>

#include <alice/library/music/defs.h>

#include <util/string/cast.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf KIND = "kind";
constexpr TStringBuf UID = "uid";

}

void ParsePlaylist(const NJson::TJsonValue& playlistJson, TMusicQueueWrapper& mq, TMusicContext& mCtx) {
    const bool hasMusicSubscription = mCtx.GetAccountStatus().GetHasMusicSubscription();

    if (auto contentInfo = TryConstructContentInfo(playlistJson)) {
        mq.SetContentInfo(*contentInfo);
    }

    // Normalize playlistId with uid instead of username, so it can be used in radio from playlist
    mq.UpdateContentId(TPlaylistId(playlistJson[UID].GetStringRobust(), playlistJson[KIND].GetStringRobust()).ToString());

    if (playlistJson[UID].GetStringRobust() == PLAYLIST_ORIGIN_OWNER_UID) {
        mq.SetEnableShots(true);
    }

    bool forceChildSafe = false;
    if (playlistJson[CHILD_CONTENT].GetBoolean()) {
        forceChildSafe = true;
    }

    const auto& tracksJson = playlistJson[TRACKS];

    if (!tracksJson.IsArray()) {
        // can happen if out offset is of bounds or no tracks in the playlist at all
        return;
    }

    const size_t startOffset = mq.GetTrackOffsetIndex();
    size_t currentOffset = mq.GetPagedFirstTrackOffsetIndex(mCtx);

    for (const auto& trackJson : tracksJson.GetArraySafe()) {
        TParseTrackParams params{.Position = currentOffset, .ForceChildSafe = forceChildSafe};
        auto track = ParseTrack(trackJson[TRACK], std::move(params));
        const bool itemAdded = mq.TryAddItem(std::move(track), hasMusicSubscription);
        if (itemAdded && currentOffset < startOffset) {
            mq.MoveTrackFromQueueToHistory();
        }
        ++currentOffset;
    }

    if (auto total = playlistJson.GetValueByPath(TOTAL_PATH)) {
        mq.SetNextTotalTracks(total->GetIntegerSafe());
    }

    const auto* ownerVerified = playlistJson.GetValueByPath("owner.verified");
    const auto* ownerUid = playlistJson.GetValueByPath("owner.uid");
    const bool isUnverified = !ownerVerified || !ownerVerified->GetBooleanSafe();
    const bool isMine = ownerUid ?
        ownerUid->GetUIntegerSafe() == FromStringWithDefault<ui64>(mCtx.GetAccountStatus().GetUid(), 0) :
        playlistJson[UID].GetStringRobust() == mCtx.GetAccountStatus().GetUid();
    mCtx.SetUnverifiedPlaylist(isUnverified && !isMine);

    mq.SetTrackOffsetIndex(0);
}

} // namespace NAlice::NHollywood::NMusic
