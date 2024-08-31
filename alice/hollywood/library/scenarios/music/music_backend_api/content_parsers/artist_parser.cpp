#include "artist_parser.h"
#include "common.h"
#include "track_parser.h"

namespace NAlice::NHollywood::NMusic {

namespace {

TMaybe<NData::NMusic::TContentInfo> TryConstructArtistContentInfo(const NJson::TJsonValue& artistJson) {
    // iterate over tracks
    for (const auto& track : artistJson[TRACKS].GetArray()) {
        // look at track's artists (at most cases there is exactly 1 artist)
        if (const auto& artists = track[ARTISTS].GetArray(); !artists.empty()) {
            // try to take the name of the first artist
            const auto& artist = *artists.begin();
            if (const auto& name = artist[NAME]; name.IsString()) {
                NData::NMusic::TContentInfo contentInfo;
                contentInfo.SetName(name.GetString());
                return contentInfo;
            }
        }
    }
    return Nothing();
}

} // namespace

void ParseArtist(const NJson::TJsonValue& artistJson, TMusicQueueWrapper& mq, const TMusicContext& mCtx) {
    if (auto contentInfo = TryConstructArtistContentInfo(artistJson)) {
        mq.SetContentInfo(*contentInfo);
    }

    const bool hasMusicSubscription = mCtx.GetAccountStatus().GetHasMusicSubscription();

    const auto& tracksJson = artistJson[TRACKS];
    if (!tracksJson.IsArray()) {
        // can happen if out offset is of bounds or an artist have no tracks at all
        return;
    }

    const size_t startOffset = mq.GetTrackOffsetIndex();
    size_t currentOffset = mq.GetPagedFirstTrackOffsetIndex(mCtx);

    for (const auto& trackJson : tracksJson.GetArraySafe()) {
        auto track = ParseTrack(trackJson, {.Position = currentOffset});
        const bool itemAdded = mq.TryAddItem(std::move(track), hasMusicSubscription);
        if (itemAdded && currentOffset < startOffset) {
            mq.MoveTrackFromQueueToHistory();
        }
        ++currentOffset;
    }

    if (auto total = artistJson.GetValueByPath(TOTAL_PATH)) {
        mq.SetNextTotalTracks(total->GetIntegerSafe());
    }
    mq.SetTrackOffsetIndex(0);
}

} // namespace NAlice::NHollywood::NMusic
