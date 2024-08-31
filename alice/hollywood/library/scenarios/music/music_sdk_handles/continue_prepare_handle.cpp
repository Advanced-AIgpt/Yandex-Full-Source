#include "common.h"
#include "continue_prepare_handle.h"
#include "requests_helper.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

void TMusicSdkContinuePrepareHandle::Do(TScenarioHandleContext& ctx) const {
    // class name is "Apply" for all request types (Apply/Commit/Continue)
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto& continueArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    const auto& searchResult = continueArgs.GetMusicSearchResult();
    TMaybe<TContentId> contentId = ContentIdFromText(searchResult.GetContentType(), searchResult.GetContentId());
    NJson::TJsonValue bassState = JsonFromString(continueArgs.GetBassScenarioState());

    bool isPlaylistRequest = false;
    const bool isSearchappRequest = CanRenderDiv2Cards(request);

    if (contentId && contentId->GetType() == TContentId_EContentType_Album && IsVariousArtistsCase(bassState)) {
        TGenreOverviewRequestHelper<ERequestPhase::Before> genreOverview(ctx, request);
        genreOverview.AddRequest(searchResult.GetAlbumGenre());
    } else if (continueArgs.HasOnDemandRequest()) {
        const TStringBuf artistId = continueArgs.GetOnDemandRequest().GetArtistId();
        TArtistBriefInfoRequestHelper<ERequestPhase::Before> artistBriefInfo(ctx, request);
        artistBriefInfo.AddRequest(artistId);
    } else if (continueArgs.HasPlaylistRequest()) {
        // we need to search for a playlist at music backend
        isPlaylistRequest = true;
        const auto& playlistRequest = continueArgs.GetPlaylistRequest();
        if (playlistRequest.GetPlaylistType() == TPlaylistRequest_EPlaylistType_Normal) {
            TPlaylistSearchRequestHelper<ERequestPhase::Before> playlistSearch(ctx, request);
            playlistSearch.AddRequest(playlistRequest.GetPlaylistName());
        } else {
            TSpecialPlaylistRequestHelper<ERequestPhase::Before> specialPlaylist(ctx, request);
            specialPlaylist.AddRequest(playlistRequest.GetPlaylistName());
        }
    } else if (contentId && contentId->GetType() == TContentId_EContentType_Playlist) {
        // we already know which playlist to play
        isPlaylistRequest = true;
        if (const auto playlistId = TPlaylistId::FromString(contentId->GetId())) {
            TPredefinedPlaylistInfoRequestHelper<ERequestPhase::Before> predefinedPlaylistInfo(ctx, request);
            predefinedPlaylistInfo.AddRequest(*playlistId);
        }
    }

    if (isPlaylistRequest && isSearchappRequest) {
        ctx.ServiceCtx.AddFlag("has_playlist_request");
    }

    // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/quasar_provider.cpp?rev=r8706594#L901
    if (request.ClientInfo().IsSearchApp() && request.Interfaces().GetCanRenderDiv2Cards()) {
        if (contentId && contentId->GetType() == TContentId_EContentType_Track) {
            const TStringBuf artistId = continueArgs.GetOnDemandRequest().GetArtistId();
            const TStringBuf trackId = contentId->GetId();

            // for play queue and other track suggests
            TArtistTracksRequestHelper<ERequestPhase::Before> artistTracks(ctx, request);
            artistTracks.AddRequest(artistId);

            // for "show the text of the song" button
            TSingleTrackRequestHelper<ERequestPhase::Before> singleTrack(ctx, request);
            singleTrack.AddRequest(trackId);
        }

        // for background gradient
        TAvatarColorsRequestHelper<ERequestPhase::Before> avatarColors(ctx);
        avatarColors.TryAddRequestFromBassState(bassState);
    }
}

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
