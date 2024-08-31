#include "entity_search_response_parsers.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/library/json/json.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

void ParsePlaylistSearchResponse(const TStringBuf response, TMusicQueueWrapper& mq, TString& playlistNameOut) {
    auto responseJson = JsonFromString(response);
    Y_ENSURE(responseJson.Has("result"));
    const auto& results = responseJson["result"]["playlists"]["results"];
    if (results.GetArray().empty()) {
        return;
    }
    const auto& firstResult = results[0];
    TContentId id;
    id.SetType(TContentId_EContentType_Playlist);
    id.SetId(TStringBuilder() << firstResult["owner"]["uid"].GetStringRobust() << ':'
                              << firstResult["kind"].GetStringRobust());
    mq.SetContentId(id);
    playlistNameOut = firstResult["title"].GetString();
}

void ParseSpecialPlaylistResponse(const TStringBuf response, TMusicQueueWrapper& mq, TString& playlistNameOut) {
    auto responseJson = JsonFromString(response);
    Y_ENSURE(responseJson.Has("result"));
    if (responseJson["result"].Has("ready") && !responseJson["result"]["ready"].GetBoolean()) {
        return;
    }
    TContentId id;
    const auto& result = responseJson["result"]["data"];
    id.SetType(TContentId_EContentType_Playlist);
    id.SetId(TStringBuilder() << result["owner"]["uid"].GetStringRobust() << ':'
                              << result["kind"].GetStringRobust());
    mq.SetContentId(id);
    playlistNameOut = result["title"].GetString();
}

void ParseNoveltyAlbumSearchResponse(const TStringBuf response, TMusicQueueWrapper& mq) {
    auto responseJson = JsonFromString(response);
    Y_ENSURE(responseJson.Has("result"));
    const auto& albums = responseJson["result"]["albums"].GetArray();
    if (albums.empty()) {
        return;
    }
    const auto& album = albums[0];
    const auto& albumId = album["id"].GetStringRobust();
    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId(albumId);
    mq.SetContentId(id);
}

} // namespace NAlice::NHollywood::NMusic
