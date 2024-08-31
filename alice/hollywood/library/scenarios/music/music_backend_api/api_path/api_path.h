#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/datetime/base.h>

#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/scenarios/music/biometry/biometry_data.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/download_info.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood::NMusic::NApiPath {

struct TApiPathRequestParams {
    const i32 PageIdx;
    const i32 PageSize;
    const bool Shuffle;
    const i32 ShuffleSeed;

    TApiPathRequestParams(i32 pageIdx, i32 pageSize, bool shuffle = false, i32 shuffleSeed = 0)
        : PageIdx(pageIdx)
        , PageSize(pageSize)
        , Shuffle(shuffle)
        , ShuffleSeed(shuffleSeed) {
    }

    TApiPathRequestParams(const TMusicQueueWrapper& mq, const TMusicContext& mCtx)
        : PageIdx(mCtx.GetFirstRequestPageSize() ? mq.GetTrackOffsetIndex() / mq.GetActualPageSize(mCtx) : mq.NextPageIndex())
        , PageSize(mq.GetActualPageSize(mCtx))
        , Shuffle(mq.GetShuffle())
        , ShuffleSeed(mq.GetShuffleSeed()) {
    }
};

// Generate path of requests to music backend API
TString ArtistBriefInfo(const TStringBuf artistId, const TStringBuf userId);
TString ArtistTracks(const TStringBuf artistId, const TStringBuf userId);
TString TrackOfArtist(const TStringBuf artistId, const TApiPathRequestParams& requestParams, const TStringBuf userId);
TString TrackIdsOfArtist(const TStringBuf artistId, const TApiPathRequestParams& requestParams, const TStringBuf userId);
TString AlbumTracks(const TStringBuf albumId, const TApiPathRequestParams& requestParams,
                    bool richTracks, const TStringBuf userId, const bool resumeStream);
TString LatestAlbumOfArtist(const TStringBuf artistId, const TStringBuf userId);
TString SingleTrack(const TStringBuf trackId, const TStringBuf userId, bool needFullInfo = false);
TString DownloadInfoMp3GetAlice(const TStringBuf trackId, const TStringBuf userId,
                                TMaybe<DownloadInfoFlag> flag = Nothing());
TString DownloadInfoHls(const TStringBuf trackId, const TStringBuf userId, TInstant ts, const TStringBuf sign);
TString PlayAudioPlays(const TString& clientNow, const TStringBuf userId);
TString SaveProgress(const TString& trackId, float positionSec, float trackLengthSec,
                     TInstant timestamp, const TString& clientNow, const TStringBuf userId);
TString LikeAlbum(const TStringBuf userId, const TStringBuf albumId);
TString LikeArtist(const TStringBuf userId, const TStringBuf artistId);
TString DislikeArtist(const TStringBuf userId, const TStringBuf artistId);
TString LikeGenre(const TStringBuf userId, const TStringBuf genreId);
TString LikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId,
                  TMaybe<TStringBuf> radioSessionId = Nothing(), TMaybe<TStringBuf> batchId = Nothing());
TString RemoveLikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId);
TString LikesTracks(const TStringBuf userId);
TString DislikesTracks(const TStringBuf userId);
TString DislikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId,
                     TMaybe<TStringBuf> radioSessionId = Nothing(), TMaybe<TStringBuf> batchId = Nothing());
TString RemoveDislikeTrack(const TStringBuf userId, const TStringBuf albumId, const TStringBuf trackId);
TString TrackSearch(const TStringBuf searchText, const TStringBuf userId);
TString PlaylistSearch(const TStringBuf userId, const TStringBuf text);
TString SpecialPlaylist(const TStringBuf userId, const TStringBuf specialPlaylist);
// FIXME(sparkle): merge two methods below into one?
TString PlaylistTracks(const TStringBuf ownerUid, const TStringBuf kind,
                       const TApiPathRequestParams& requestParams, bool richTracks,
                       const TStringBuf userId);
TString PlaylistTracksForMusicSdk(const TStringBuf userId, const TStringBuf ownerUid, const TStringBuf kind);
TString RadioFeedback(const TStringBuf radioStationId, const TStringBuf batchId={}, const TStringBuf radioSessionId={});
TString RadioTracks(const TStringBuf radioStationId, const TVector<TStringBuf>& queue, bool newRadioSession);
TString RadioSessionFeedback(const TStringBuf radioSessionId);
TString GenerativeStream(const TStringBuf generativeStationId);
TString GenerativeFeedback(const TStringBuf generativeStationId, const TStringBuf streamId);
TString FmRadioRankedList(const TStringBuf userId, const TStringBuf ip, float lat, float lon);
TString InfiniteFeed(const TStringBuf userId);
TString ChildrenLandingCatalogue(const TStringBuf userId);

// Name of the handle 'after track' was acknowledged to be inappropriate, but it was too late
// What actually the handle returns are shots that can be played BEFORE next track
// https://wiki.yandex-team.ru/users/psmcd/alicedj-api-bjekenda-muzyki/
TString AfterTrack(const TStringBuf userId, const TStringBuf from,
                   const TStringBuf context, const TStringBuf  contextItem,
                   const TStringBuf prevTrackId, const TStringBuf nextTrackId);
TString ShotsFeedback(const TStringBuf userId);
TString ShotsLikeDislikeFeedback(const TStringBuf userId, const TStringBuf shotId,
                                 const TStringBuf prevTrackId, const TStringBuf nextTrackId,
                                 const TStringBuf context, const TStringBuf contextItem,
                                 const TStringBuf from, bool isLike);
TString GenreOverview(const TStringBuf genre, const int artistsCount, const TStringBuf userId);

// Generate path of requests to third party backend API
TString ConstructMetaPathFromCoverUri(TStringBuf coverUri);

// Some new handlers are POST and require body instead of CGI
using TPathAndBody = std::pair<TString, TString>;

TPathAndBody RadioNewSessionPathAndBody(const TVector<TStringBuf>& radioStationIds, const TVector<TStringBuf>& queue,
                                        const TBiometryData& biometryData, const NScenarios::TUserPreferences_EFiltrationMode filtrationMode,
                                        const TExpFlags& flags, const TString& startFromTrackId, const bool fromSpecified, const bool useIchwill);
TPathAndBody RadioSessionTracksPathAndBody(const TStringBuf radioSessionId, const TVector<TStringBuf>& queue,
                                           const TExpFlags& flags, const bool useIchwill, const TMaybe<TString> djData = Nothing());

} // namespace NAlice::NHollywood::NMusic::NApiPath
