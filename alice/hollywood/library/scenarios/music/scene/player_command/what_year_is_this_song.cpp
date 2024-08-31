#include "common.h"
#include "what_year_is_this_song.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/scenario_meta_processor.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "alice.music.what_year_is_this_song",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_WHAT_YEAR_IS_THIS_SONG,
};

} // namespace

TMusicScenarioScenePlayerCommandWhatYearIsThisSong::TMusicScenarioScenePlayerCommandWhatYearIsThisSong(const TScenario* owner)
    : TScene{owner, "player_command_what_year_is_this_song"}
{
}

TRetMain TMusicScenarioScenePlayerCommandWhatYearIsThisSong::Main(const TMusicScenarioSceneArgsPlayerCommandWhatYearIsThisSong&,
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
