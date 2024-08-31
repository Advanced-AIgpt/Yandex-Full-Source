#include "common.h"
#include "continue_playlist_setdown_handle.h"
#include "requests_helper.h"

#include <alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder/nlg_data_builder.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

void TMusicSdkContinuePlaylistSetdownHandle::Do(TScenarioHandleContext& ctx) const {
    // this node is for div2-data only
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const auto request = TScenarioApplyRequestWrapper(requestProto, ctx.ServiceCtx);
    Y_ENSURE(CanRenderDiv2Cards(request));

    const auto& continueArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    const auto& searchResult = continueArgs.GetMusicSearchResult();
    TMaybe<TContentId> contentId = ContentIdFromText(searchResult.GetContentType(), searchResult.GetContentId());

    const TPlaylistSearchRequestHelper<ERequestPhase::After> playlistSearch{ctx};
    const TSpecialPlaylistRequestHelper<ERequestPhase::After> specialPlaylist{ctx};
    const TPredefinedPlaylistInfoRequestHelper<ERequestPhase::After> predefinedPlaylistInfo{ctx};

    bool playlistIdParsed = false;
    TMaybe<TString> playlistId;
    TMaybe<TString> coverUri;
    if (playlistSearch.HasResponse()) {
        const auto& rawResponse = *playlistSearch.TryGetResponse();
        playlistIdParsed = true;
        playlistId = NUsualPlaylist::FindPlaylistId(rawResponse);
        coverUri = NUsualPlaylist::ConstructCoverUri(rawResponse);
    } else if (specialPlaylist.HasResponse()) {
        const auto& rawResponse = *specialPlaylist.TryGetResponse();
        playlistIdParsed = true;
        playlistId = NSpecialPlaylist::FindPlaylistId(rawResponse);
        coverUri = NSpecialPlaylist::ConstructCoverUri(rawResponse);
    } else if (predefinedPlaylistInfo.HasResponse()) {
        Y_ENSURE(contentId && contentId->GetType() == TContentId_EContentType_Playlist);
        coverUri = NPredefinedPlaylist::ConstructCoverUri(*predefinedPlaylistInfo.TryGetResponse());
    }

    if (coverUri.Defined()) {
        // for background gradient
        NAlice::NHollywood::NMusic::TAvatarColorsRequestHelper<ERequestPhase::Before> avatarColors{ctx};
        avatarColors.AddRequest(*coverUri);
    }

    if (playlistId.Defined()) {
        if (const auto playlistIdObj = TPlaylistId::FromString(*playlistId)) {
            // for "similar playlists" field
            TPlaylistInfoRequestHelper<ERequestPhase::Before> playlistInfo{ctx, request};
            playlistInfo.AddRequest(*playlistIdObj);
        }
    } else if (playlistIdParsed) {
        // failed to parse playlist id! redirect to Ya.Music site
        ctx.ServiceCtx.AddFlag(PLAYLIST_SETDOWN_RESPONSE_FAILED_APPHOST_FLAG);
    }
}

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
