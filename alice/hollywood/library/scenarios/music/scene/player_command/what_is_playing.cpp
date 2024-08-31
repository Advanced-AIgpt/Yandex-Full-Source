#include "common.h"
#include "what_is_playing.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/scenario_meta_processor.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TCommandInfo COMMAND_INFO{
    .ProductScenarioName = "music_what_is_playing",
    .Intent = "personal_assistant.scenarios.music_what_is_playing",
    .NlgTemplate = NHollywood::NMusic::TEMPLATE_PLAYER_WHAT_IS_PLAYING,
};

} // namespace

TMusicScenarioScenePlayerCommandWhatIsPlaying::TMusicScenarioScenePlayerCommandWhatIsPlaying(const TScenario* owner)
    : TScene{owner, "player_command_what_is_playing"}
{
}

TRetMain TMusicScenarioScenePlayerCommandWhatIsPlaying::Main(const TMusicScenarioSceneArgsPlayerCommandWhatIsPlaying&,
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
