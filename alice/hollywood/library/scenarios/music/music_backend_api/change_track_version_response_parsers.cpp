#include "entity_search_response_parsers.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/library/json/json.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

namespace {

TMaybe<TString> FindTrackIdByVersion(const NJson::TJsonValue::TArray& tracks, TStringBuf version) {
    for (const auto& track : tracks) {
        if (track["version"].GetStringSafe("") == version) {
            return track["id"].GetStringRobust();
        }
    }
    return Nothing();
}

} // namespace

void ParseTrackFullInfoResponse(const TStringBuf response, TMusicQueueWrapper& mq, const TStringBuf trackVersion) {
    auto responseJson = JsonFromString(response);
    Y_ENSURE(responseJson.Has("result"));
    const auto& result = responseJson["result"];
    if (!result.Has("otherVersions")) {
        return;
    }

    const auto& otherVersions = result["otherVersions"];
    // logic from https://st.yandex-team.ru/MUSICBACKEND-9763#62bee23701c17f6e71ccbb30
    if (trackVersion == "original" && otherVersions.Has("another")) {
        TContentId id;
        id.SetType(TContentId_EContentType_Track);
        if (const auto trackId = FindTrackIdByVersion(otherVersions["another"].GetArray(), "Original Version")) {
            id.SetId(*trackId);
        } else if (const auto trackId = FindTrackIdByVersion(otherVersions["another"].GetArray(), "AlbumVersion")) {
            id.SetId(*trackId);
        } else if (const auto trackId = FindTrackIdByVersion(otherVersions["another"].GetArray(), "")) {
            id.SetId(*trackId);
        } else {
            id.SetId(otherVersions["another"][0]["id"].GetStringRobust());
        }
        mq.SetContentId(id);
    } else if (trackVersion == "live" && otherVersions.Has("live")) {
        TContentId id;
        id.SetType(TContentId_EContentType_Track);
        id.SetId(otherVersions["live"][0]["id"].GetStringRobust());
        mq.SetContentId(id);
    } else if (trackVersion == "remix" && otherVersions.Has("remix")) {
        TContentId id;
        id.SetType(TContentId_EContentType_Track);
        id.SetId(otherVersions["remix"][0]["id"].GetStringRobust());
        mq.SetContentId(id);
    }
}

void ParseTrackSearchResponse(const TStringBuf response, TMusicQueueWrapper& mq) {
    auto responseJson = JsonFromString(response);
    Y_ENSURE(responseJson.Has("result"));
    const auto& results = responseJson["result"]["tracks"]["results"];
    for (const auto& result : results.GetArray()) {
        if (!mq.HasTrackInHistory(result["id"].GetStringRobust())) {
            TContentId id;
            id.SetType(TContentId_EContentType_Track);
            id.SetId(result["id"].GetStringRobust());
            mq.SetContentId(id);
            return;
        }
    }
}

} // namespace NAlice::NHollywood::NMusic
