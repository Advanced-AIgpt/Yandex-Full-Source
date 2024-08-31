#pragma once

#include <alice/hollywood/library/scenarios/music/requests_helper/requests_helper.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

// ARTIST_BRIEF_INFO
class TBeforeArtistBriefInfoRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_ARTIST_BRIEF_INFO_REQUEST_ITEM; }
    TStringBuf Name() const override { return "ArtistBriefInfo"; };

    void AddRequest(const TStringBuf artistId);
};

class TAfterArtistBriefInfoRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_ARTIST_BRIEF_INFO_RESPONSE_ITEM; }

    TMaybe<int> GetLikesCount() const;
    TVector<const NJson::TJsonValue*> FindRawOtherAlbums(const int originalAlbumId) const;
    TVector<const NJson::TJsonValue*> FindRawSimilarArtists(size_t limit = std::numeric_limits<size_t>::max()) const;
    TVector<NJson::TJsonValue> FindPreparedSimilarArtists() const;
};

template<ERequestPhase RequestPhase>
using TArtistBriefInfoRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeArtistBriefInfoRequestHelper, TAfterArtistBriefInfoRequestHelper>;


// SINGLE_TRACK
class TBeforeSingleTrackRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_SINGLE_TRACK_REQUEST_ITEM; }
    TStringBuf Name() const override { return "SingleTrack"; };

    void AddRequest(const TStringBuf trackId);
};

class TAfterSingleTrackRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_SINGLE_TRACK_RESPONSE_ITEM; }

    bool LyricsAvailable() const;
    TString ArtistName() const;
    TString Title() const;
};

template<ERequestPhase RequestPhase>
using TSingleTrackRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeSingleTrackRequestHelper, TAfterSingleTrackRequestHelper>;


// ARTIST_TRACKS
class TBeforeArtistTracksRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_ARTIST_TRACKS_REQUEST_ITEM; }
    TStringBuf Name() const override { return "ArtistTracks"; };

    void AddRequest(const TStringBuf artistId);
};

class TAfterArtistTracksRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_ARTIST_TRACKS_RESPONSE_ITEM; }

    TVector<const NJson::TJsonValue*> FindRawOtherTracks(const int originalTrackId) const;
    TVector<NJson::TJsonValue> FindPreparedOtherTracks(const int originalTrackId) const;
};

template<ERequestPhase RequestPhase>
using TArtistTracksRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeArtistTracksRequestHelper, TAfterArtistTracksRequestHelper>;


// GENRE_OVERVIEW
class TBeforeGenreOverviewRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_GENRE_OVERVIEW_REQUEST_ITEM; }
    TStringBuf Name() const override { return "GenreOverview"; };

    void AddRequest(const TStringBuf genre);
};

class TAfterGenreOverviewRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_GENRE_OVERVIEW_RESPONSE_ITEM; }

    TVector<const NJson::TJsonValue*> FindRawTopArtists() const;
};

template<ERequestPhase RequestPhase>
using TGenreOverviewRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeGenreOverviewRequestHelper, TAfterGenreOverviewRequestHelper>;

// PLAYLIST_SEARCH
class TBeforePlaylistSearchRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_PLAYLIST_SEARCH_REQUEST_ITEM; }
    TStringBuf Name() const override { return "PlaylistSearch"; };

    void AddRequest(const TStringBuf playlistName);
};

class TAfterPlaylistSearchRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_PLAYLIST_SEARCH_RESPONSE_ITEM; }
};

template<ERequestPhase RequestPhase>
using TPlaylistSearchRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforePlaylistSearchRequestHelper, TAfterPlaylistSearchRequestHelper>;


// SPECIAL_PLAYLIST
class TBeforeSpecialPlaylistRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_SPECIAL_PLAYLIST_REQUEST_ITEM; }
    TStringBuf Name() const override { return "SpecialPlaylist"; };

    void AddRequest(const TStringBuf playlistName);
};

class TAfterSpecialPlaylistRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_SPECIAL_PLAYLIST_RESPONSE_ITEM; }
};

template<ERequestPhase RequestPhase>
using TSpecialPlaylistRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeSpecialPlaylistRequestHelper, TAfterSpecialPlaylistRequestHelper>;


// PLAYLIST_INFO
class TBeforePlaylistInfoRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_PLAYLIST_INFO_REQUEST_ITEM; }
    TStringBuf Name() const override { return "PlaylistInfo"; };

    void AddRequest(const TPlaylistId& playlistId);
};

class TAfterPlaylistInfoRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_PLAYLIST_INFO_RESPONSE_ITEM; }
    const TMaybe<NJson::TJsonValue>& TryGetResponse() const; // may return patched playlist object

    TVector<NJson::TJsonValue> FindPreparedSimilarPlaylists(const TPlaylistId& originalPlaylistId) const;

private:
    mutable bool ResponsePatched_ = false;
};

template<ERequestPhase RequestPhase>
using TPlaylistInfoRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforePlaylistInfoRequestHelper, TAfterPlaylistInfoRequestHelper>;


// PREDEFINED_PLAYLIST_INFO (is actually the same request as PLAYLIST_INFO)
class TBeforePredefinedPlaylistInfoRequestHelper : public TBeforePlaylistInfoRequestHelper {
public:
    using TBeforePlaylistInfoRequestHelper::TBeforePlaylistInfoRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_PREDEFINED_PLAYLIST_INFO_REQUEST_ITEM; }
    TStringBuf Name() const override { return "PredefinedPlaylistInfo"; };
};

class TAfterPredefinedPlaylistInfoRequestHelper : public TAfterPlaylistInfoRequestHelper {
public:
    using TAfterPlaylistInfoRequestHelper::TAfterPlaylistInfoRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_PREDEFINED_PLAYLIST_INFO_RESPONSE_ITEM; }
};

template<ERequestPhase RequestPhase>
using TPredefinedPlaylistInfoRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforePredefinedPlaylistInfoRequestHelper, TAfterPredefinedPlaylistInfoRequestHelper>;


// MISC
namespace NImpl {

TVector<const NJson::TJsonValue*> FindArtistBriefInfoSimilarArtists(const NJson::TJsonValue& briefInfoObj, const size_t limit = std::numeric_limits<size_t>::max());
TVector<const NJson::TJsonValue*> FindArtistBriefInfoOtherAlbums(const NJson::TJsonValue& briefInfoObj, const int originalAlbumId);
TVector<const NJson::TJsonValue*> FindArtistOtherTracks(const NJson::TJsonValue& tracksObj, const int originalTrackId);
TVector<const NJson::TJsonValue*> FindGenreOverviewTopArtists(const NJson::TJsonValue& genreOverviewObj);

} // namespace NImpl

} // NAlice::NHollywood::NMusic::NMusicSdk
