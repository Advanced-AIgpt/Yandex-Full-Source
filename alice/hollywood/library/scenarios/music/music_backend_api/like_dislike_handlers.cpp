#include "like_dislike_handlers.h"

#include <alice/hollywood/library/framework/framework_migration.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

namespace NAlice::NHollywood::NMusic {

namespace {

NAppHostHttp::THttpRequest MakeMusicLikeDislikeRequest(const NScenarios::TRequestMeta& meta,
                                              const TClientInfo& clientInfo, TRTLogger& logger,
                                              const TString& commandPath, TString commandName,
                                              const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo)
{
    return TMusicRequestBuilder(commandPath, meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, std::move(commandName))
        .SetMethod(NAppHostHttp::THttpRequest_EMethod_Post)
        .Build();
}

template<typename T>
void MusicLikeDislikeEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response, const TStringBuf handle, const T& resultChecker) {
    try {
        Y_ENSURE(response.Has("result"));
        resultChecker(response["result"]);
        LOG_INFO(logger) << "Response from like/dislike " << handle << " handle is OK";
    } catch(...) {
        ythrow yexception() << "Failed to ensure music like/dislike " << handle << " response. Exception: "
                            << CurrentExceptionMessage() << ". Response: " << JsonToString(response);
    }
}

void EnsureResultIsOk(const NJson::TJsonValue& result) {
    const auto& resultStr = result.GetStringSafe();
    Y_ENSURE(resultStr == "ok");
}

} // namespace

NAppHostHttp::THttpRequest MakeMusicLikeAlbumRequest(const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
                                            TRTLogger& logger, const TStringBuf userId, 
                                            const TStringBuf albumId, const bool enableCrossDc,
                                            const TMusicRequestModeInfo& musicRequestModeInfo)
{
    return MakeMusicLikeDislikeRequest(meta, clientInfo, logger, NApiPath::LikeAlbum(userId, albumId),
                                       "LikeAlbum", enableCrossDc, musicRequestModeInfo);
}

NAppHostHttp::THttpRequest MakeMusicLikeDislikeArtistRequest(const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
                                                    TRTLogger& logger, const TStringBuf userId,
                                                    const bool isLikeCommand, const TStringBuf artistId,
                                                    const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo)
{
    if (isLikeCommand) {
        return MakeMusicLikeDislikeRequest(meta, clientInfo, logger, NApiPath::LikeArtist(userId, artistId),
                                           "LikeArtist", enableCrossDc, musicRequestModeInfo);
    } else {
        return MakeMusicLikeDislikeRequest(meta, clientInfo, logger, NApiPath::DislikeArtist(userId, artistId),
                                           "DislikeArtist", enableCrossDc, musicRequestModeInfo);
    }
}

NAppHostHttp::THttpRequest MakeMusicLikeGenreRequest(const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
                                            TRTLogger& logger, const TStringBuf userId,
                                            const TStringBuf genreId, const bool enableCrossDc,
                                            const TMusicRequestModeInfo& musicRequestModeInfo)
{
    return MakeMusicLikeDislikeRequest(meta, clientInfo, logger, NApiPath::LikeGenre(userId, genreId),
                                       "LikeGenre", enableCrossDc, musicRequestModeInfo);
}

NAppHostHttp::THttpRequest MakeMusicLikeDislikeTrackRequest(const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
                                                   TRTLogger& logger, const TStringBuf userId,
                                                   const bool isLikeCommand, const TStringBuf trackId, const TStringBuf albumId,
                                                   const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo,
                                                   TMaybe<TStringBuf> radioSessionId, TMaybe<TStringBuf> batchId)
{
    if (isLikeCommand) {
        return MakeMusicLikeDislikeRequest(meta, clientInfo, logger, NApiPath::LikeTrack(userId, albumId, trackId, radioSessionId, batchId),
                                           "Like", enableCrossDc, musicRequestModeInfo);
    } else {
        return MakeMusicLikeDislikeRequest(meta, clientInfo, logger, NApiPath::DislikeTrack(userId, albumId, trackId, radioSessionId, batchId),
                                           "Dislike", enableCrossDc, musicRequestModeInfo);
    }
}

NAppHostHttp::THttpRequest MakeMusicLikeDislikeTrackFromQueueRequest(const TScenarioBaseRequestWrapper& request,
                                                            const NScenarios::TRequestMeta& meta,
                                                            const TClientInfo& clientInfo, TRTLogger& logger,
                                                            const TStringBuf userId, bool isLikeCommand,
                                                            const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo)
{
    // We use the original scenario state for queue, cause in
    // the current scenario state track may be changed by now
    TScenarioState originalScState;
    Y_ENSURE(ReadScenarioState(request.BaseRequestProto(), originalScState));
    TryInitPlaybackContextBiometryOptions(logger, originalScState);
    const TMusicQueueWrapper mq(logger, *originalScState.MutableQueue());
    const auto& currentItem = mq.CurrentItem();
    const auto trackId = currentItem.GetTrackId();
    const auto albumId = currentItem.GetTrackInfo().GetAlbumId();

    TMaybe<TStringBuf> radioSessionId;
    TMaybe<TStringBuf> batchId;
    if (mq.IsRadio()) {
        radioSessionId = mq.GetRadioSessionId();
        batchId = mq.GetRadioBatchId();
    }

    return MakeMusicLikeDislikeTrackRequest(meta, clientInfo, logger, userId, isLikeCommand, trackId, albumId, enableCrossDc,
                                            musicRequestModeInfo, radioSessionId, batchId);
}

THttpProxyRequestItemPair WrapMusicLikeDislikeRequest(NAppHostHttp::THttpRequest&& request) {
    // TODO(jan-fazli): rename MUSIC_LIKE_REQUEST_ITEM, cause it is now also used for dislikes
    return { std::move(request), TString{MUSIC_LIKE_REQUEST_ITEM} };
}

void MusicLikeDislikeAlbumEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response) {
    MusicLikeDislikeEnsureResponse(logger, response, "album", EnsureResultIsOk);
}

void MusicLikeDislikeArtistEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response) {
    MusicLikeDislikeEnsureResponse(logger, response, "artist", EnsureResultIsOk);
}

void MusicLikeDislikeGenreEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response) {
    MusicLikeDislikeEnsureResponse(logger, response, "genre", EnsureResultIsOk);
}

void MusicLikeDislikeTrackEnsureResponse(TRTLogger& logger, const NJson::TJsonValue& response) {
    MusicLikeDislikeEnsureResponse(logger, response, "track", [](const NJson::TJsonValue& result) {
        Y_ENSURE(result.Has("revision"));
        const auto& revision = result["revision"].GetIntegerSafe();
        Y_ENSURE(revision >= 0);
    });
}

} // namespace NAlice::NHollywood::NMusic
