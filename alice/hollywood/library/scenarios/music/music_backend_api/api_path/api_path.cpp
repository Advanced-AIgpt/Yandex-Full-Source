#include "api_path.h"

#include <alice/hollywood/library/scenarios/music/time_util/time_util.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/track_album_id.h>
#include <alice/library/json/json.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/cgiparam/cgiparam.h>

#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NMusic::NApiPath {

namespace {

constexpr TStringBuf CLIENT_REMOTE_TYPE_ALICE = "alice";
constexpr TStringBuf CLIENT_REMOTE_TYPE_PULT = "pult";

TCgiParameters GetFmRadioCgiParams(const TStringBuf ip, float lat, float lon) {
    TCgiParameters params;
    params.InsertUnescaped("ip", ip);
    params.InsertUnescaped("lat", ToString(lat));
    params.InsertUnescaped("lon", ToString(lon));
    return params;
}

TString LikeOrDislikeTrack(const TStringBuf what, const TStringBuf userId, const TStringBuf albumId,
                           const TStringBuf trackId, TMaybe<TStringBuf> radioSessionId, TMaybe<TStringBuf> batchId)
{
    TTrackAlbumId trackAlbumId{trackId, albumId};
    auto sb = TStringBuilder{} << "/users/" << userId << "/" << what << "/tracks/add?track-id=" << trackAlbumId.ToString()
                               << "&__uid=" << userId;
    if (radioSessionId) {
        sb << "&radioSessionId=" << *radioSessionId;
    }
    if (batchId) {
        sb << "&batchId=" << *batchId;
    }
    return sb;
}

} // namespace

TString ArtistBriefInfo(const TStringBuf artistId, const TStringBuf userId) {
    return TStringBuilder{} << "/artists/" << artistId << "/brief-info" << "?__uid=" << userId;
}

TString ArtistTracks(const TStringBuf artistId, const TStringBuf userId) {
    return TStringBuilder{} << "/artists/" << artistId << "/tracks" << "?__uid=" << userId;
}

TString TrackOfArtist(const TStringBuf artistId, const TApiPathRequestParams& requestParams, const TStringBuf userId) {
    TCgiParameters params;
    params.InsertUnescaped("page", ToString(requestParams.PageIdx));
    params.InsertUnescaped("pageSize", ToString(requestParams.PageSize));

    if (requestParams.Shuffle) {
        params.InsertUnescaped("shuffle", "true");
        params.InsertUnescaped("shuffleSeed", ToString(requestParams.ShuffleSeed));
    }

    params.InsertUnescaped("__uid", userId);

    return TStringBuilder{} << "/artists/" << artistId << "/tracks?" << params.Print();
}

TString TrackIdsOfArtist(const TStringBuf artistId, const TApiPathRequestParams& requestParams, const TStringBuf userId) {
    TCgiParameters params;
    params.InsertUnescaped("page", ToString(requestParams.PageIdx));
    params.InsertUnescaped("pageSize", ToString(requestParams.PageSize));

    if (requestParams.Shuffle) {
        params.InsertUnescaped("shuffle", "true");
        params.InsertUnescaped("shuffleSeed", ToString(requestParams.ShuffleSeed));
    }

    params.InsertUnescaped("__uid", userId);

    return TStringBuilder{} << "/artists/" << artistId << "/track-ids?" << params.Print();
}

TString AlbumTracks(const TStringBuf albumId, const TApiPathRequestParams& requestParams,
                    bool richTracks, const TStringBuf userId, bool resumeStream) {
    TCgiParameters params;
    params.InsertUnescaped("page", ToString(requestParams.PageIdx));
    params.InsertUnescaped("pageSize", ToString(requestParams.PageSize));
    params.InsertUnescaped("richTracks", (richTracks ? "true" : "false"));

    if (requestParams.Shuffle) {
        params.InsertUnescaped("shuffle", "true");
        params.InsertUnescaped("shuffleSeed", ToString(requestParams.ShuffleSeed));
    }

    if (resumeStream) {
        params.InsertUnescaped("resumeStream", "true");
    }

    params.InsertUnescaped("__uid", userId);

    return TStringBuilder{} << "/albums/" << albumId << "/with-tracks?" << params.Print();
}

TString LatestAlbumOfArtist(const TStringBuf artistId, const TStringBuf userId) {
    return TStringBuilder{} << "/artists/" << artistId
                            << "/direct-albums?page=0&page-size=1&sort-by=year&sort-order=desc&__uid=" << userId;
}

TString SingleTrack(const TStringBuf trackId, const TStringBuf userId, bool needFullInfo) {
    return TStringBuilder{} << "/tracks/" << trackId << (needFullInfo ? "/full-info" : "") << "?__uid=" << userId;
}

TString DownloadInfoMp3GetAlice(const TStringBuf trackId, const TStringBuf userId, TMaybe<DownloadInfoFlag> flag) {
    auto sb = TStringBuilder{} << "/tracks/" << trackId << "/download-info?isAliceRequester=true&__uid=" << userId;
    if (flag) {
        sb << "&formatFlags=" << ToString(*flag);
    }
    return sb;
}

TString DownloadInfoHls(const TStringBuf trackId, const TStringBuf userId, TInstant ts, const TStringBuf sign) {
    TCgiParameters params;
    params.InsertUnescaped("__uid", userId);
    params.InsertUnescaped("ts", ToString(ts.Seconds()));
    params.InsertUnescaped("sign", sign);
    params.InsertUnescaped("can_use_streaming", "true");
    return TStringBuilder{} << "/tracks/" << trackId << "/download-info?" << params.Print();
}

TString PlayAudioPlays(const TString& clientNow, const TStringBuf userId) {
    TCgiParameters params;
    params.InsertUnescaped("__uid", userId);

    if (clientNow) {
        params.InsertUnescaped("client-now", clientNow);
    }

    return "/plays?" + params.Print();
}

TString SaveProgress(const TString& trackId, float positionSec, float trackLengthSec,
                     TInstant timestamp, const TString& clientNow, const TStringBuf userId) {
    TCgiParameters params;
    params.InsertUnescaped("__uid", userId);

    if (clientNow) {
        params.InsertUnescaped("client-now", clientNow);
    }

    params.InsertUnescaped("trackId", trackId);
    if (abs(positionSec) < EPS) {
        positionSec = 0.1f;
    }
    if (abs(trackLengthSec) > EPS) {
        params.InsertUnescaped("trackLengthSec", ToString(trackLengthSec));
    }
    params.InsertUnescaped("positionSec", ToString(positionSec));
    params.InsertUnescaped("timestamp", FormatTInstant(timestamp));

    return "/streams/progress/save-current?" + params.Print();
}

TString LikeAlbum(const TStringBuf userId, const TStringBuf albumId) {
    return TStringBuilder{} << "/users/" << userId << "/likes/albums/add?album-id=" << albumId
                            << "&__uid=" << userId;
}

TString LikeArtist(const TStringBuf userId, const TStringBuf artistId) {
    return TStringBuilder{} << "/users/" << userId << "/likes/artists/add?artist-id=" << artistId
                            << "&__uid=" << userId;
}

TString DislikeArtist(const TStringBuf userId, const TStringBuf artistId) {
    return TStringBuilder{} << "/users/" << userId << "/dislikes/artists/add?artist-id=" << artistId
                            << "&__uid=" << userId;
}

TString LikeGenre(const TStringBuf userId, const TStringBuf genreId) {
    return TStringBuilder{} << "/users/" << userId << "/likes/genres/add?genre=" << genreId
                            << "&__uid=" << userId;
}

TString LikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId,
                  TMaybe<TStringBuf> radioSessionId, TMaybe<TStringBuf> batchId) {
    return LikeOrDislikeTrack("likes", userId, albumId, trackId, radioSessionId, batchId);
}

TString RemoveLikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId) {
    TTrackAlbumId trackAlbumId{trackId, albumId};
    return TString::Join("/users/", userId, "/likes/tracks/", trackAlbumId.ToString(), "/remove?__uid=", userId);
}

TString LikesTracks(const TStringBuf userId) {
    return TString::Join("/users/", userId, "/likes/tracks?__uid=", userId);
}

TString DislikesTracks(const TStringBuf userId) {
    return TString::Join("/users/", userId, "/dislikes/tracks?__uid=", userId);
}

TString DislikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId,
                     TMaybe<TStringBuf> radioSessionId, TMaybe<TStringBuf> batchId) {
    return LikeOrDislikeTrack("dislikes", userId, albumId, trackId, radioSessionId, batchId);
}

TString RemoveDislikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId) {
    TTrackAlbumId trackAlbumId{trackId, albumId};
    return TString::Join("/users/", userId, "/dislikes/tracks/", trackAlbumId.ToString(), "/remove?__uid=", userId);
}

TString TrackSearch(const TStringBuf searchText, const TStringBuf userId) {
    return TStringBuilder{} << "/search?type=track&page=0&text=" << CGIEscapeRet(searchText)
                            << "&__uid=" << userId;
}

TString PlaylistSearch(const TStringBuf userId, const TStringBuf text) {
    return TStringBuilder{} << "/search?type=playlist&page=0&text=" << CGIEscapeRet(text)
                            << "&__uid=" << userId;
}

TString SpecialPlaylist(const TStringBuf userId, const TStringBuf specialPlaylist) {
    return TStringBuilder{} << "/playlists/personal/" << specialPlaylist << "?__uid=" << userId;
}

TString PlaylistTracks(const TStringBuf ownerUid, const TStringBuf kind,
                       const TApiPathRequestParams& requestParams, bool richTracks,
                       const TStringBuf userId) {
    TCgiParameters params;
    params.InsertUnescaped("page", ToString(requestParams.PageIdx));
    params.InsertUnescaped("pageSize", ToString(requestParams.PageSize));
    params.InsertUnescaped("rich-tracks", (richTracks ? "true" : "false"));

    if (requestParams.Shuffle) {
        params.InsertUnescaped("shuffle", "true");
        params.InsertUnescaped("shuffleSeed", ToString(requestParams.ShuffleSeed));
    }

    params.InsertUnescaped("__uid", userId);

    if (kind == PLAYLIST_LIKE_KIND) {
        params.InsertUnescaped("trackMetaType", "music");
    }

    return TStringBuilder() << "/users/" << ownerUid << "/playlists/" << kind << "?" << params.Print();
}

TString PlaylistTracksForMusicSdk(const TStringBuf userId, const TStringBuf ownerUid, const TStringBuf kind) {
    return TString::Join("/users/", ownerUid, "/playlists/", kind,
                         "?rich-tracks=false&withSimilarsLikesCount=true",
                         "&__uid=", userId);
}

TString RadioFeedback(const TStringBuf radioStationId, const TStringBuf batchId, const TStringBuf radioSessionId) {
    auto result = TStringBuilder() << "/station/" << radioStationId << "/feedback";
    TCgiParameters params;
    if (!batchId.Empty()) {
        params.InsertUnescaped("batch-id", batchId);
    }
    if (!radioSessionId.Empty()) {
        params.InsertUnescaped("radio-session-id", radioSessionId);
    }
    const auto paramsStr = params.Print();
    if (paramsStr.empty()) {
        return result;
    } else {
        return result << '?' << paramsStr;
    }
}

TString RadioTracks(const TStringBuf radioStationId, const TVector<TStringBuf>& queue, bool newRadioSession) {
    auto result = TStringBuilder() << "/station/" << radioStationId << "/tracks";
    TCgiParameters params;
    if (!queue.empty()) {
        params.InsertUnescaped("queue", JoinSeq(",", queue));
    }
    if (newRadioSession) {
        params.InsertUnescaped("new-radio-session", "true");
    }
    const auto paramsStr = params.Print();
    if (paramsStr.empty()) {
        return result;
    } else {
        return result << '?' << paramsStr;
    }
}

TString RadioSessionFeedback(const TStringBuf radioSessionId) {
    return TString::Join("/session/", radioSessionId, "/feedback");
}

TString GenerativeStream(const TStringBuf generativeStationId) {
    auto result = TStringBuilder() << "/station/" << generativeStationId << "/stream";
    return result;
}

TString GenerativeFeedback(const TStringBuf generativeStationId, const TStringBuf streamId) {
    auto result = TStringBuilder() << "/station/" << generativeStationId << "/feedback";
    TCgiParameters params;
    params.InsertUnescaped("streamId", streamId);
    return result << '?' << params.Print();
}

TString FmRadioRankedList(const TStringBuf userId, const TStringBuf ip, float lat, float lon) {
    auto result = TStringBuilder() << "/radio-stream/ranked/list?__uid=" << userId;
    return result << '&' << GetFmRadioCgiParams(ip, lat, lon).Print();
}

TString InfiniteFeed(const TStringBuf userId) {
    return TString::Join("/infinite-feed?landingType=navigator&supportedBlocks=generic&__uid=", userId);
}

TString ChildrenLandingCatalogue(const TStringBuf userId) {
    return TString::Join("/children-landing/catalogue?requestedBlocks=CATEGORIES_TAB&__uid=", userId);
}

TString AfterTrack(const TStringBuf userId, const TStringBuf from,
                   const TStringBuf context, const TStringBuf contextItem,
                   const TStringBuf prevTrackId, const TStringBuf nextTrackId) {
    TCgiParameters params;
    params.InsertUnescaped("types", "shot");
    params.InsertUnescaped("__uid", userId);
    params.InsertUnescaped("from", from);
    if (!context.empty() && !contextItem.empty()) {
        params.InsertUnescaped("context", context);
        params.InsertUnescaped("contextItem", contextItem);
    }
    if (!prevTrackId.empty()) {
        params.InsertUnescaped("prevTrackId", prevTrackId);
    }
    if (!nextTrackId.empty()) {
        params.InsertUnescaped("nextTrackId", nextTrackId);
    }
    return "/after-track?" + params.Print();
}

TString ShotsFeedback(const TStringBuf userId) {
    return TString("/shots/feedback?__uid=") + userId;
}

TString ShotsLikeDislikeFeedback(const TStringBuf userId, const TStringBuf shotId,
                                 const TStringBuf prevTrackId, const TStringBuf nextTrackId,
                                 const TStringBuf context, const TStringBuf contextItem,
                                 const TStringBuf from, bool isLike) {
    TCgiParameters params;
    params.InsertUnescaped("shotId", shotId);
    params.InsertUnescaped("__uid", userId);
    params.InsertUnescaped("from", from);
    if (!context.empty() && !contextItem.empty()) {
        params.InsertUnescaped("context", context);
        params.InsertUnescaped("contextItem", contextItem);
    }
    if (!prevTrackId.empty()) {
        params.InsertUnescaped("prevTrackId", prevTrackId);
    }
    if (!nextTrackId.empty()) {
        params.InsertUnescaped("nextTrackId", nextTrackId);
    }

    return TStringBuilder() << "/users/" << userId << "/" << (isLike ? "likes" : "dislikes") << "/shots/add?" << params.Print();
}

TString GenreOverview(const TStringBuf genre, const int artistsCount, const TStringBuf userId) {
    TCgiParameters params;
    params.InsertUnescaped("__uid", userId);
    params.InsertUnescaped("albums-count", "0");
    params.InsertUnescaped("artists-count", ToString(artistsCount));
    params.InsertUnescaped("genre", genre);
    params.InsertUnescaped("promotions-count", "0");
    params.InsertUnescaped("tracks-count", "0");

    return TStringBuilder{} << "/genre-overview?" << params.Print();
}

TString ConstructMetaPathFromCoverUri(TStringBuf coverUri) {
    // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/quasar_provider.cpp?rev=r8709318#L1417
    TStringBuf handler;
    TStringBuf spaceName;
    TStringBuf groupId;
    TStringBuf imageName;

    coverUri = coverUri.RBefore('/');
    coverUri.RSplit('/', coverUri, imageName);
    coverUri.RSplit('/', coverUri, groupId);
    coverUri.RSplit('/', coverUri, spaceName);
    spaceName.Split('-', handler, spaceName);

    return TString::Join("/", handler, "info-", spaceName, "/", groupId, "/", imageName, "/meta");
}

TPathAndBody RadioNewSessionPathAndBody(const TVector<TStringBuf>& radioStationIds, const TVector<TStringBuf>& queue,
                                        const TBiometryData& biometryData, const NScenarios::TUserPreferences_EFiltrationMode filtrationMode,
                                        const TExpFlags& flags, const TString& startFromTrackId, const bool fromSpecified, const bool useIchwill)
{
    NJson::TJsonValue json(NJson::EJsonValueType::JSON_MAP);

    Y_ENSURE(!radioStationIds.empty());
    auto& seedsJson = json["seeds"];
    for (const auto id : radioStationIds) {
        seedsJson.AppendValue(id);
    }

    if (!queue.empty()) {
        auto& queueJson = json["queue"];
        for (const auto id : queue) {
            queueJson.AppendValue(id);
        }
    }

    json["includeTracksInResponse"] = true;
    if (fromSpecified) {
        json["clientRemoteType"] = CLIENT_REMOTE_TYPE_PULT;
    } else {
        json["clientRemoteType"] = CLIENT_REMOTE_TYPE_ALICE;

    }
    json["incognito"] = biometryData.IsIncognitoUser;
    json["child"] = biometryData.IsChild || filtrationMode == NScenarios::TUserPreferences_EFiltrationMode_Safe;
    json["allowExplicit"] = filtrationMode == NScenarios::TUserPreferences_EFiltrationMode_NoFilter;

    if (useIchwill) {
        // XXX(amullanurov): Вот здесь потом добавим поле json["use_ichwill"], https://st.yandex-team.ru/MUSICBACKEND-8439
        json["aliceExperiments"]["force_ichwill"] = "1";
    }

    if (!flags.empty()) {
        auto& flagsJson = json["aliceExperiments"];
        for (const auto& [key, value] : flags) {
            flagsJson[key] = value.Defined() ? *value : "1";
        }
    }

    if (!startFromTrackId.empty()) {
        json["trackToStartFrom"] = startFromTrackId;
    }

    return {"/session/new", JsonToString(json)};
}

TPathAndBody RadioSessionTracksPathAndBody(const TStringBuf radioSessionId, const TVector<TStringBuf>& queue,
                                           const TExpFlags& flags, const bool useIchwill, const TMaybe<TString> djData) {
    NJson::TJsonValue json(NJson::EJsonValueType::JSON_MAP);

    if (!queue.empty()) {
        auto& queueJson = json["queue"];
        for (const auto id : queue) {
            queueJson.AppendValue(id);
        }
    }

    if (useIchwill) {
        json["aliceExperiments"]["force_ichwill"] = "1";
    }

    if (!flags.empty()) {
        auto& flagsJson = json["aliceExperiments"];
        for (const auto& [key, value] : flags) {
            flagsJson[key] = value.Defined() ? *value : "1";
        }
    }

    if (djData.Defined()) {
        json["djData"] = *djData.Get();
    }

    TString path = TString::Join("/session/", radioSessionId, "/tracks");
    return {std::move(path), JsonToString(json)};
}

} // namespace NAlice::NHollywood::NMusic::NApiPath
