#include "common.h"
#include "what_album_is_this_song_from.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/scenario_meta_processor.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "alice.music.what_album_is_this_song_from",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_WHAT_ALBUM_IS_THIS_SONG_FROM,
};

} // namespace

TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom::TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom(const TScenario* owner)
    : TScene{owner, "player_command_what_album_is_this_song_from"}
{
}

TRetMain TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom::Main(const TMusicScenarioSceneArgsPlayerCommandWhatAlbumIsThisSongFrom&,
                                                                       const TRunRequest& request,
                                                                       TStorage& storage,
                                                                       const TSource& source) const
{
    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};
    TCommonRenderData renderData = TScenarioMetaProcessor{requestData}
        .SetCommandInfo(COMMAND_INFO)
        .Process();
    return TReturnValueRender(&CommonRender, renderData.RenderArgs, std::move(renderData.RunFeatures));
}

} // namespace NAlice::NHollywoodFw::NMusic
