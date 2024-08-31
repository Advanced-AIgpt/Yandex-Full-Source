#include "common.h"
#include "songs_by_this_artist.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/music_args.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/scenario_meta_processor.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "alice.music.songs_by_this_artist",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_SONGS_BY_THIS_ARTIST,
};

constexpr std::array<TStringBuf, 6> NON_MUSIC_TYPES = {
    TStringBuf("fm_radio"),
    TStringBuf("shot"),
    TStringBuf("audiobook"),
    TStringBuf("podcast"),
    TStringBuf("podcast-episode"),
    TStringBuf("generative"),
};
constexpr std::array<TStringBuf, 2> NON_MUSIC_GENRES = {
    TStringBuf("fairytales"),
    TStringBuf("poemsforchildren"),
};

} // namespace

TMusicScenarioScenePlayerCommandSongsByThisArtist::TMusicScenarioScenePlayerCommandSongsByThisArtist(const TScenario* owner)
    : TScene{owner, "player_command_songs_by_this_artist"}
{
}

TRetMain TMusicScenarioScenePlayerCommandSongsByThisArtist::Main(const TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist&,
                                                                       const TRunRequest& request,
                                                                       TStorage& storage,
                                                                       const TSource& source) const
{
    TMaybe<TString> artistId;
    bool isNonMusic = false;
    const auto currentItemProcessor = [&artistId, &isNonMusic](TMusicScenarioRenderArgsCommon&,
                                                  const NHollywood::NMusic::TQueueItem& currentItem)
    {
        if (const auto trackArtistId = currentItem.GetTrackInfo().GetArtistId(); !trackArtistId.Empty()) {
            artistId = trackArtistId;
        }
        if (IsIn(NON_MUSIC_TYPES, currentItem.GetType()) ||
            IsIn(NON_MUSIC_GENRES, currentItem.GetTrackInfo().GetGenre()))
        {
            isNonMusic = true;
        }
    };

    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};

    TCommonRenderData renderData = TScenarioMetaProcessor{requestData}
        .SetCommandInfo(COMMAND_INFO)
        .SetCurrentItemProcessor(currentItemProcessor)
        .Process();

    NHollywood::TMusicArguments continueArgs;
    if (artistId.Defined() && !isNonMusic) {
        NHollywood::TMusicArguments continueArgs = MakeMusicArguments(request, /* isNewContentRequestedByUser= */ true);
        auto& musicSearchResult = *continueArgs.MutableMusicSearchResult();
        musicSearchResult.SetContentType("artist");
        musicSearchResult.SetContentId(*artistId);
        return TReturnValueContinue(continueArgs, std::move(renderData.RunFeatures));
    } else if (isNonMusic) {
        renderData.RenderArgs.MutableNlgData()->MutableContext()->SetIsNonMusic(true);
    }
    return TReturnValueRender(&CommonRender, renderData.RenderArgs, std::move(renderData.RunFeatures));
}

TRetSetup TMusicScenarioScenePlayerCommandSongsByThisArtist::ContinueSetup(
    const TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist& sceneArgs,
    const TContinueRequest& request,
    const TStorage&) const
{
    Y_UNUSED(sceneArgs);
    Y_UNUSED(request);
    return TReturnValueDo();
}

TRetContinue TMusicScenarioScenePlayerCommandSongsByThisArtist::Continue(
    const TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist& sceneArgs,
    const TContinueRequest& request,
    TStorage& storage,
    const TSource& source) const
{
    Y_UNUSED(sceneArgs);
    Y_UNUSED(request);
    Y_UNUSED(storage);
    Y_UNUSED(source);
    return TReturnValueDo();
}

} // namespace NAlice::NHollywoodFw::NMusic
