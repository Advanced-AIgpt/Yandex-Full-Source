#include "play_less.h"

namespace NAlice::NHollywoodFw::NMusic {

TMusicScenarioScenePlayLess::TMusicScenarioScenePlayLess(const TScenario* owner)
    : TScene{owner, "play_less"}
{
    SetProductScenarioName("placeholders");
}

TRetMain TMusicScenarioScenePlayLess::Main(const TMusicScenarioSceneArgsPlayLess& args, const TRunRequest&, TStorage&, const TSource&) const {
    TRunFeatures features;
    features.SetIntentName("personal_assistant.scenarios.music_play_less");
    return TReturnValueRender("music_play_less", "render", args, std::move(features));
}

} // namespace NAlice::NHollywoodFw::NMusic
