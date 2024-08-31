#include "find_track_idx_response_parsers.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/library/json/json.h>

#include <util/generic/xrange.h>
#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const size_t MaxPossiblePageSize = 50;

void ParseAlbumFindTrackIdxResponse(const NJson::TJsonValue& responseJson, const TStringBuf trackId,
                                    TMusicQueueWrapper& mq)
{
    Y_ENSURE(responseJson.Has("result"));

    if (mq.GetShuffle()) {
        const auto& tracksJson = responseJson["result"]["tracks"];
        if (tracksJson.IsArray()) {
            const auto& tracks = tracksJson.GetArraySafe();
            for (size_t i = 0; i < tracks.size(); i++) {
                if (tracksJson[i]["id"].GetStringRobust() == trackId) {
                    mq.SetTrackOffsetIndex(i);
                    return;
                }
            }
        }
        return;
    }

    const auto& volumesJson = responseJson["result"]["volumes"];

    if (volumesJson.IsArray()) {
        size_t i = 0;
        for (const auto& volumeJson : volumesJson.GetArraySafe()) {
            for (const auto& trackJson : volumeJson.GetArraySafe()) {
                if (trackJson["id"].GetStringRobust() == trackId) {
                    mq.SetTrackOffsetIndex(i);
                    return;
                }
                i++;
            }
        }
        return;
    }
}

void ParseArtistFindTrackIdxResponse(const NJson::TJsonValue& responseJson, const TStringBuf trackId,
                                     TMusicQueueWrapper& mq)
{
    Y_ENSURE(responseJson.Has("result"));

    const auto& tracksJson = responseJson["result"];
    if (!tracksJson.IsArray()) {
        // artist has no tracks
        return;
    }

    const auto& tracks = tracksJson.GetArraySafe();
    for (size_t i = 0; i < tracks.size(); i++) {
        if (tracks[i] == trackId) {
            mq.SetTrackOffsetIndex(i);
            return;
        }
    }
}

void ParsePlaylistFindTrackIdxResponse(const NJson::TJsonValue& responseJson, const TStringBuf trackId,
                                       TMusicQueueWrapper& mq)
{
    Y_ENSURE(responseJson.Has("result"));

    const auto& tracksJson = responseJson["result"]["tracks"];
    if (!tracksJson.IsArray()) {
        // no tracks in the playlist
        return;
    }

    const auto& tracks = tracksJson.GetArraySafe();
    for (size_t i = 0; i < tracks.size(); i++) {
        auto& track = tracks[i];
        if (track["id"].GetStringRobust() == trackId) {
            // If pageSize = 20 and tracks originalIndex are ..., 99, 101, 103, ...,
            // we want to have TrackOffsetIndex(101) = 100, TrackOffsetIndex(103) = 101.
            // So for trackId with originalIndex = 103, we find its offset as 100 + number of tracks in range [100, 103)
            const ui32 originalIndex = track["originalIndex"].GetIntegerSafe();

            const auto [firstPageSize, nextPageIdx] = FindOptimalPageParameters(mq, originalIndex);

            // 103 -> 100, 119 -> 100, 120 -> 120, if pageSize = 20
            const ui32 pageStartOriginalIndex = originalIndex - originalIndex % firstPageSize;
            ui32 pageStartIndex = i;

            while (pageStartIndex > 0 &&
                   tracks[pageStartIndex - 1]["originalIndex"].GetIntegerSafe() >= pageStartOriginalIndex)
            {
                pageStartIndex--;
            }

            mq.SetTrackOffsetIndex(pageStartOriginalIndex + i - pageStartIndex);
            return;
        }
    }
}

} // empty namespace

std::pair<size_t, size_t> FindOptimalPageParameters(const TMusicQueueWrapper& mq, const size_t trackIdx) {
    // upperBound = min(x): x % mq.Config().PageSize() == 0 and x > trackOffsetIndex
    const size_t upperBound = (trackIdx / mq.Config().PageSize + 1) * mq.Config().PageSize;

    // nextPageIdx is calculated based on upperBound.
    // While downloading tracks we pretend to use appropriate nextPageIndex which suits config PageSize.
    // CurrentPageIdx will be set to nextPageIdx after parsing
    const size_t nextPageIdx = upperBound / mq.Config().PageSize - 1;

    const size_t minFirstPageSize = 30;

    // firstPageSize == min(x): x >= minFirstPageSize, upperBound % x == 0.
    // Tracks from trackOffsetIndex to upperBound - 1 and previous mq.Config().HistorySize tracks
    // should be in history
    for (const size_t firstPageSize : xrange(minFirstPageSize, MaxPossiblePageSize + 1)) {
        if (upperBound % firstPageSize == 0) {
            return {firstPageSize, nextPageIdx};
        }
    }

    // Default page size is doubled pageSize for minimizing chance of lack of tracks in history
    return {mq.Config().PageSize * 2, nextPageIdx};
}

void ParseFindTrackIdxResponse(const TStringBuf response, const TFindTrackIdxRequest& request,
                               TMusicQueueWrapper& mq)
{
    auto responseJson = JsonFromString(response);

    if (request.GetContentType() == TFindTrackIdxRequest_EContentType_Album) {
        if (request.GetShouldUseResumeFrom()) {
            ParseAlbumFindTrackIdxResponse(responseJson,
                responseJson["result"]["resumeFrom"]["trackId"].GetStringRobust(), mq);
        } else {
            ParseAlbumFindTrackIdxResponse(responseJson, request.GetTrackId(), mq);
        }
    } else if (request.GetContentType() == TFindTrackIdxRequest_EContentType_Artist) {
        ParseArtistFindTrackIdxResponse(responseJson, request.GetTrackId(), mq);
    } else if (request.GetContentType() == TFindTrackIdxRequest_EContentType_Playlist) {
        ParsePlaylistFindTrackIdxResponse(responseJson, request.GetTrackId(), mq);
    } else {
        ythrow yexception() << "Unexpected ContentId.Type="
                            << TFindTrackIdxRequest_EContentType_Name(request.GetContentType())
                            << " for parsing find_track_idx response";
    }
}

} // namespace NAlice::NHollywood::NMusic
