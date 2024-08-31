#include "remove_like.h"

#include <alice/hollywood/library/scenarios/music/scene/common/common_args.h>
#include <alice/hollywood/library/scenarios/music/scene/common/like_status.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/request.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

using NAlice::NHollywood::NMusic::TMusicQueueWrapper;

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "audio.music.remove_like",
    .ActionInfo = TCommandInfo::TActionInfo{
        .Id = "remove_like",
        .Name = "remove like",
        .HumanReadable = "Трек не будет помечаться как понравившийся",
    }
};

}

TMusicScenarioScenePlayerCommandRemoveLike::TMusicScenarioScenePlayerCommandRemoveLike(const TScenario* owner)
    : TScene{owner, "player_command_remove_like"}
{
}

TRetMain TMusicScenarioScenePlayerCommandRemoveLike::Main(const TMusicScenarioSceneArgsPlayerCommandRemoveLike& sceneArgs,
                                                          const TRunRequest& request,
                                                          TStorage& storage,
                                                          const TSource& source) const
{
    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};
    TScenarioStateData state{requestData};

    TCommonRenderData renderData;
    renderData.FillRunFeatures(requestData);
    requestData.FillAnalyticsInfo(COMMAND_INFO, state);

    const auto* contentId = TryGetContentId(sceneArgs.GetCommonArgs());
    if (TryGetTrackAlbumId(state.MusicQueue, contentId).Defined()) {
        static const auto commitArgs = google::protobuf::Empty{};
        return TReturnValueCommit(&CommonRender, renderData.RenderArgs, commitArgs, std::move(renderData.RunFeatures));
    } else {
        return TReturnValueRender(&CommonRender, renderData.RenderArgs, std::move(renderData.RunFeatures));
    }
}

TRetSetup TMusicScenarioScenePlayerCommandRemoveLike::CommitSetup(const TMusicScenarioSceneArgsPlayerCommandRemoveLike& sceneArgs,
                                                                  const TCommitRequest& request,
                                                                  const TStorage& storage) const
{
    TScenarioRequestData requestData{.Request = request, .Storage = const_cast<TStorage&>(storage)};
    TScenarioStateData state{requestData};

    const auto* contentId = TryGetContentId(sceneArgs.GetCommonArgs());
    const auto trackAlbumId = TryGetTrackAlbumId(state.MusicQueue, contentId);
    Y_ENSURE(trackAlbumId.Defined());

    const TStringBuf userId = sceneArgs.GetCommonArgs().GetAccountStatus().GetUid();
    const TString path = NHollywood::NMusic::NApiPath::RemoveLikeTrack(userId, trackAlbumId->AlbumId, trackAlbumId->TrackId);
    const NAppHostHttp::THttpRequest httpRequest = PrepareCommonHttpRequest(request, userId, path, "RemoveLike");

    TSetup setup{request};
    setup.AttachRequest(NHollywood::NMusic::MUSIC_REMOVE_LIKE_REQUEST_ITEM, httpRequest);
    setup.AttachRequest("hw_music_arguments", google::protobuf::Empty{}); // TODO(sparkle): remove it after graphs+HW release
    return setup;
}

TRetCommit TMusicScenarioScenePlayerCommandRemoveLike::Commit(const TMusicScenarioSceneArgsPlayerCommandRemoveLike&,
                                                              const TCommitRequest&,
                                                              TStorage&,
                                                              const TSource&) const
{
    return TReturnValueSuccess{};
}

} // namespace NAlice::NHollywoodFw::NMusic
