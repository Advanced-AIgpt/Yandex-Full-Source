#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

NAppHostHttp::THttpRequest MakeMusicLikeAlbumRequest(
    const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
    TRTLogger& logger, const TStringBuf userId, const TStringBuf albumId,
    const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo);

NAppHostHttp::THttpRequest MakeMusicLikeDislikeArtistRequest(
    const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
    TRTLogger& logger, const TStringBuf userId,
    const bool isLikeCommand, const TStringBuf artistId,
    const bool enableCrossDcc, const TMusicRequestModeInfo& musicRequestModeInfo);

NAppHostHttp::THttpRequest MakeMusicLikeGenreRequest(
    const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
    TRTLogger& logger, const TStringBuf userId,
    const TStringBuf genreId, const bool enableCrossDcc,
    const TMusicRequestModeInfo& musicRequestModeInfo);

NAppHostHttp::THttpRequest MakeMusicLikeDislikeTrackRequest(
    const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
    TRTLogger& logger, const TStringBuf userId,
    const bool isLikeCommand, const TStringBuf trackId, const TStringBuf albumId,
    const bool enableCrossDcc, const TMusicRequestModeInfo& musicRequestModeInfo,
    TMaybe<TStringBuf> radioSessionId = Nothing(), TMaybe<TStringBuf> batchId = Nothing());

NAppHostHttp::THttpRequest MakeMusicLikeDislikeTrackFromQueueRequest(
    const TScenarioBaseRequestWrapper& request, const NScenarios::TRequestMeta& meta,
    const TClientInfo& clientInfo, TRTLogger& logger,
    const TStringBuf userId, const bool isLikeCommand,
    const bool enableCrossDcc, const TMusicRequestModeInfo& musicRequestModeInfo);

THttpProxyRequestItemPair WrapMusicLikeDislikeRequest(NAppHostHttp::THttpRequest&& request);

void MusicLikeDislikeAlbumEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response);
void MusicLikeDislikeArtistEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response);
void MusicLikeDislikeGenreEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response);
void MusicLikeDislikeTrackEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response);

} // namespace NAlice::NHollywood::NMusic
