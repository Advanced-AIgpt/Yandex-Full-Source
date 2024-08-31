#include "requests_helper.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/playlist_proxy_prepare.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder/nlg_data_builder.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

constexpr TStringBuf PLAYLIST_LIKE_KIND = "3";
constexpr TStringBuf PLAYLIST_LIKE_TITLE = "Мне нравится";
constexpr TStringBuf PLAYLIST_LIKE_COVER_TYPE = "pic";
constexpr TStringBuf PLAYLIST_LIKE_COVER_URI = "avatars.yandex.net/get-music-user-playlist/28719/291769700.1017.3715/%%";

const NJson::TJsonValue* TryGetTrackObj(const TMaybe<NJson::TJsonValue>& response) {
    if (response.Empty()) {
        return nullptr;
    }
    const auto& result = (*response)["result"].GetArray();
    if (result.empty()) {
        return nullptr;
    }
    return &(*result.begin());
}

TVector<const NJson::TJsonValue*> FindObjects(const NJson::TJsonValue& obj, const TStringBuf path,
        const size_t limit = std::numeric_limits<size_t>::max(), const TMaybe<int> ignoredId = Nothing())
{
    const auto* pathObj = obj.GetValueByPath(path);
    Y_ENSURE(pathObj);

    TVector<const NJson::TJsonValue*> result;
    for (const auto& itemObj : pathObj->GetArray()) {
        if (ignoredId.Defined() &&
            (itemObj["id"].GetInteger() == *ignoredId || itemObj["id"].GetString() == ToString(*ignoredId)))
        {
            continue;
        }
        result.push_back(&itemObj);
        if (result.size() >= limit) {
            break;
        }
    }
    return result;
}

} // namespace

void TBeforeArtistBriefInfoRequestHelper::AddRequest(const TStringBuf artistId) {
    Y_ENSURE(MusicArgs_);
    const TString path = NApiPath::ArtistBriefInfo(artistId, MusicArgs_->GetAccountStatus().GetUid());
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

TMaybe<int> TAfterArtistBriefInfoRequestHelper::GetLikesCount() const {
    if (const auto& jsonObj = TryGetResponse()) {
        if (const auto* likesCount = jsonObj->GetValueByPath("result.artist.likesCount")) {
            return likesCount->GetIntegerRobust();
        }
    }
    return Nothing();
}

TVector<const NJson::TJsonValue*> TAfterArtistBriefInfoRequestHelper::FindRawSimilarArtists(size_t limit) const {
    if (const auto& jsonObj = TryGetResponse()) {
        return NImpl::FindArtistBriefInfoSimilarArtists(*jsonObj, limit);
    }
    return {};
}

TVector<NJson::TJsonValue> TAfterArtistBriefInfoRequestHelper::FindPreparedSimilarArtists() const {
    TVector<NJson::TJsonValue> artists;
    for (const auto* v : FindRawSimilarArtists()) {
        NJson::TJsonValue artistInfo;
        artistInfo["artistId"] = (*v)["id"].GetStringRobust();
        artistInfo["name"] = (*v)["name"].GetStringRobust();
        artistInfo["imageUri"] = ConstructCoverUri((*v)["cover"]["uri"].GetStringRobust());
        artists.push_back(std::move(artistInfo));
    }
    return artists;
}

TVector<const NJson::TJsonValue*> TAfterArtistBriefInfoRequestHelper::FindRawOtherAlbums(const int originalAlbumId) const {
    if (const auto& jsonObj = TryGetResponse()) {
        return NImpl::FindArtistBriefInfoOtherAlbums(*jsonObj, originalAlbumId);
    }
    return {};
}

void TBeforeSingleTrackRequestHelper::AddRequest(const TStringBuf trackId) {
    Y_ENSURE(MusicArgs_);
    const TString path = NApiPath::SingleTrack(trackId, MusicArgs_->GetAccountStatus().GetUid());
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

bool TAfterSingleTrackRequestHelper::LyricsAvailable() const {
    if (const auto* trackObj = TryGetTrackObj(TryGetResponse())) {
        return (*trackObj)["lyricsAvailable"].GetBooleanSafe(/* defaultValue= */ false);
    }
    return false;
}

TString TAfterSingleTrackRequestHelper::ArtistName() const {
    const auto* trackObj = TryGetTrackObj(TryGetResponse());
    Y_ENSURE(trackObj);
    return (*trackObj)["artists"][0]["name"].GetStringRobust();
}

TString TAfterSingleTrackRequestHelper::Title() const {
    const auto* trackObj = TryGetTrackObj(TryGetResponse());
    Y_ENSURE(trackObj);
    return (*trackObj)["title"].GetStringRobust();
}

void TBeforeArtistTracksRequestHelper::AddRequest(const TStringBuf artistId) {
    Y_ENSURE(MusicArgs_);
    const TString path = NApiPath::ArtistTracks(artistId, MusicArgs_->GetAccountStatus().GetUid());
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

TVector<const NJson::TJsonValue*> TAfterArtistTracksRequestHelper::FindRawOtherTracks(const int originalTrackId) const {
    if (const auto& tracksResponse = TryGetResponse()) {
        return NImpl::FindArtistOtherTracks(*tracksResponse, originalTrackId);
    }
    return {};
}

TVector<NJson::TJsonValue> TAfterArtistTracksRequestHelper::FindPreparedOtherTracks(const int originalTrackId) const {
    const auto rawTracks = FindRawOtherTracks(originalTrackId);
    TVector<NJson::TJsonValue> tracks;
    for (const auto* v : rawTracks) {
        NJson::TJsonValue trackInfo;
        trackInfo["trackId"] = (*v)["id"].GetStringRobust();
        trackInfo["title"] = (*v)["title"].GetStringRobust();
        trackInfo["imageUri"] = ConstructCoverUri((*v)["coverUri"].GetStringRobust());
        tracks.push_back(std::move(trackInfo));
    }
    return tracks;
}

void TBeforeGenreOverviewRequestHelper::AddRequest(const TStringBuf genre) {
    Y_ENSURE(MusicArgs_);
    const TString path = NApiPath::GenreOverview(genre, /* artistsCount= */ 3, MusicArgs_->GetAccountStatus().GetUid());
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

TVector<const NJson::TJsonValue*> TAfterGenreOverviewRequestHelper::FindRawTopArtists() const {
    if (const auto& genreOverviewResponse = TryGetResponse()) {
        return NImpl::FindGenreOverviewTopArtists(*genreOverviewResponse);
    }
    return {};
}

void TBeforePlaylistSearchRequestHelper::AddRequest(const TStringBuf playlistName) {
    Y_ENSURE(MusicArgs_);
    const TStringBuf uid = MusicArgs_->GetAccountStatus().GetUid();
    const auto path = NApiPath::PlaylistSearch(uid, playlistName);
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

void TBeforeSpecialPlaylistRequestHelper::AddRequest(const TStringBuf playlistName) {
    Y_ENSURE(MusicArgs_);
    const TStringBuf uid = MusicArgs_->GetAccountStatus().GetUid();
    const auto path = NApiPath::SpecialPlaylist(uid, ConvertSpecialPlaylistId(playlistName));
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

void TBeforePlaylistInfoRequestHelper::AddRequest(const TPlaylistId& playlistId) {
    Y_ENSURE(MusicArgs_);
    const TStringBuf uid = MusicArgs_->GetAccountStatus().GetUid();
    const auto path = NApiPath::PlaylistTracksForMusicSdk(uid, playlistId.Owner, playlistId.Kind);
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

const TMaybe<NJson::TJsonValue>& TAfterPlaylistInfoRequestHelper::TryGetResponse() const {
    const auto& response = TAfterHttpJsonRequestHelper::TryGetResponse();
    if (!ResponsePatched_) {
        ResponsePatched_ = true;
        NJson::TJsonValue& value = const_cast<NJson::TJsonValue&>(*response);

        if (value["result"]["kind"].GetStringRobust() == PLAYLIST_LIKE_KIND) {
            LOG_INFO(Ctx_.Ctx.Logger()) << "We will patch \"like\" playlist response's cover and title";
            value["result"]["title"] = PLAYLIST_LIKE_TITLE;
            value["result"]["cover"]["type"] = PLAYLIST_LIKE_COVER_TYPE;
            value["result"]["cover"]["uri"] = PLAYLIST_LIKE_COVER_URI;
        }
    }
    return response;
}

TVector<NJson::TJsonValue>
TAfterPlaylistInfoRequestHelper::FindPreparedSimilarPlaylists(const TPlaylistId& originalPlaylistId) const {
    TVector<NJson::TJsonValue> playlists;
    if (const auto& jsonObj = TryGetResponse()) {
        for (const auto& v : (*jsonObj)["result"]["similarPlaylists"].GetArray()) {
            const auto& kind = v["kind"].GetStringRobust();
            if (kind.Empty() || kind == originalPlaylistId.Kind) {
                continue;
            }
            NJson::TJsonValue playlistInfo;
            playlistInfo["kind"] = kind;
            playlistInfo["title"] = v["title"].GetStringRobust();
            playlistInfo["imageUri"] = ConstructCoverUri(v["ogImage"].GetStringRobust());
            playlists.push_back(std::move(playlistInfo));
        }
    }
    return playlists;
}

namespace NImpl {

TVector<const NJson::TJsonValue*> FindArtistBriefInfoSimilarArtists(const NJson::TJsonValue& briefInfoObj, const size_t limit) {
    return FindObjects(briefInfoObj, "result.similarArtists", limit);
}

TVector<const NJson::TJsonValue*> FindArtistBriefInfoOtherAlbums(const NJson::TJsonValue& briefInfoObj, const int originalAlbumId) {
    return FindObjects(briefInfoObj, "result.albums", /* limit= */ 3, /* ignoredId= */ originalAlbumId);
}

TVector<const NJson::TJsonValue*> FindArtistOtherTracks(const NJson::TJsonValue& tracksObj, const int originalTrackId) {
    return FindObjects(tracksObj, "result.tracks", /* limit= */ std::numeric_limits<size_t>::max(), /* ignoredId= */ originalTrackId);
}

TVector<const NJson::TJsonValue*> FindGenreOverviewTopArtists(const NJson::TJsonValue& genreOverviewObj) {
    return FindObjects(genreOverviewObj, "result.artists", /* limit= */ 3);
}

} // namespace NImpl

} // NAlice::NHollywood::NMusic::NMusicSdk
