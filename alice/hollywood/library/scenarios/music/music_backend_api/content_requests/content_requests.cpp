#include "content_requests.h"

#include <alice/hollywood/library/scenarios/music/util/util.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>

#include <alice/megamind/protos/common/location.pb.h>
#include <alice/protos/data/scenario/music/dj_request_data.pb.h>
#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

NAppHostHttp::THttpRequest PrepareContentRequest(const TScenarioBaseRequestWrapper& request, const TMusicQueueWrapper& mq, const TMusicContext& mCtx,
                                        TAtomicSharedPtr<IRequestMetaProvider> metaProvider, TRTLogger& logger, const TBiometryData& biometryData,
                                        const TStringBuf requesterUserId, const TMaybe<NApiPath::TApiPathRequestParams> customRequestParams, 
                                        const TMusicArguments_EPlayerCommand playerCommand)
{
    const NScenarios::TUserPreferences_EFiltrationMode filtrationMode = request.BaseRequestProto().GetUserPreferences().GetFiltrationMode();
    const TExpFlags& flags = request.ExpFlags();
    const auto& id = mq.ContentId().GetId();
    TString path;
    TMaybe<TString> body;
    auto useOAuth = false;
    auto authMethod = EAuthMethod::UserId;
    switch (mq.ContentId().GetType()) {
        case TContentId_EContentType_Track:
            path = NApiPath::SingleTrack(id, requesterUserId);
            break;
        case TContentId_EContentType_Album: {
            const auto params = customRequestParams.Defined() ? *customRequestParams : NApiPath::TApiPathRequestParams(mq, mCtx);
            path = NApiPath::AlbumTracks(id, params, /* richTracks = */ true, requesterUserId,
                                         mCtx.GetFindTrackIdxRequest().GetShouldUseResumeFrom());
            break;
        }
        case TContentId_EContentType_Artist: {
            const auto params = customRequestParams.Defined() ? *customRequestParams : NApiPath::TApiPathRequestParams(mq, mCtx);
            path = NApiPath::TrackOfArtist(id, params, requesterUserId);
            break;
        }
        case TContentId_EContentType_Playlist: {
            const auto params = customRequestParams.Defined() ? *customRequestParams : NApiPath::TApiPathRequestParams(mq, mCtx);
            auto playlistId = TPlaylistId::FromString(id);
            path = NApiPath::PlaylistTracks(playlistId->Owner, playlistId->Kind,
                                            params, /* richTracks = */ true, requesterUserId);
            break;
        }
        case TContentId_EContentType_Generative: {
            path = NApiPath::GenerativeStream(id);
            useOAuth = true;
            authMethod = EAuthMethod::OAuth;
            break;
        }
        case TContentId_EContentType_FmRadio: {
            const TStringBuf ip = request.BaseRequestProto().GetOptions().GetClientIP();
            const float lat = request.BaseRequestProto().GetLocation().GetLat();
            const float lon = request.BaseRequestProto().GetLocation().GetLon();
            path = NApiPath::FmRadioRankedList(requesterUserId, ip, lat, lon);
            break;
        }
        case TContentId_EContentType_Radio: {
            TVector<TStringBuf> queue;

            TMaybe<TString> b64RequestData;
            if (mq.HasCurrentItem() && playerCommand == TMusicArguments_EPlayerCommand_NextTrack && !IsUgcTrackId(TStringBuf(mq.CurrentItem().GetTrackId()))) {
                TString serializedRequestData;
                NAlice::NData::NMusic::TDjMusicRequestData music_request_data;
                music_request_data.SetSkippedTrackId(std::stoi(mq.CurrentItem().GetTrackId()));
                if (music_request_data.SerializeToString(&serializedRequestData)) {
                    b64RequestData = Base64Encode(TStringBuf(serializedRequestData));
                }
            }

            if (mq.HasCurrentItem() &&
                !IsUgcTrackId(TStringBuf(mq.CurrentItem().GetTrackId())) &&
                mq.CurrentItem().GetType() != ContentTypeToText(TContentId_EContentType_FmRadio) &&
                mq.CurrentItem().GetType() != ContentTypeToText(TContentId_EContentType_Generative))
            {
                // The music recommender knows about all played tracks from the History
                // (from "OnPlayStartedCallback" callback),
                // but it may not know about the current track, because it hasn't been sent to the user yet.
                // We send the "queue" (the list of pending tracks) to the music recommender
                queue.push_back(TStringBuf(mq.CurrentItem().GetTrackId()));
            }
            if (mCtx.GetFirstPlay()) {
                auto [reqPath, reqBody] = NApiPath::RadioNewSessionPathAndBody(mq.ContentIdsValues(), queue, biometryData,
                                                                               filtrationMode, flags, mq.GetStartFromTrackId(),
                                                                               !mq.GetPlaybackContext().GetFrom().empty(),
                                                                               mq.GetPlaybackContext().GetUseIchwill());
                path = std::move(reqPath);
                body = std::move(reqBody);
            } else {
                auto [reqPath, reqBody] = NApiPath::RadioSessionTracksPathAndBody(mq.GetRadioSessionId(), queue, flags,
                                                                                  mq.GetPlaybackContext().GetUseIchwill(), b64RequestData);
                path = std::move(reqPath);
                body = std::move(reqBody);
            }
            useOAuth = true; // TODO(vitvlkv): Do not use OAuth after https://st.yandex-team.ru/MUSICBACKEND-5955
            authMethod = EAuthMethod::OAuth;
            break;
        }
        case TContentId_EContentType_TContentId_EContentType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TContentId_EContentType_TContentId_EContentType_INT_MAX_SENTINEL_DO_NOT_USE_:
            // Make static analyzer happy
            break;
    }
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    auto musicRequestModeInfo = TMusicRequestModeInfoBuilder()
                            .SetAuthMethod(authMethod)
                            .SetRequestMode(GetRequestMode(biometryData))
                            .SetOwnerUserId(mCtx.GetAccountStatus().GetUid())
                            .SetRequesterUserId(requesterUserId)
                            .BuildAndMove();
    auto builder = TMusicRequestBuilder(path, metaProvider, request.ClientInfo(), logger, enableCrossDc, musicRequestModeInfo, "ContentTracks");
    if (useOAuth) {
        builder.SetUseOAuth();
    }
    if (body) {
        builder.SetMethod(NAppHostHttp::THttpRequest_EMethod_Post);
        builder.SetBody(*body);
    }
    return std::move(builder).BuildAndMove(/* logVerbose = */ true);
}

} // namespace NAlice::NHollywood::NMusic
