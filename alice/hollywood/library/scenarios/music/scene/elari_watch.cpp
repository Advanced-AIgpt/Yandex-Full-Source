#include "elari_watch.h"

namespace NAlice::NHollywoodFw::NMusic {

TMusicScenarioSceneElariWatch::TMusicScenarioSceneElariWatch(const TScenario* owner)
    : TScene{owner, "elari_watch"}
{
}

TRetMain TMusicScenarioSceneElariWatch::Main(const TMusicScenarioSceneArgsElariWatch& args, const TRunRequest&, TStorage&, const TSource&) const {
    return TReturnValueRender("music_play", "elari_watch_not_supported", args);
}

} // namespace NAlice::NHollywoodFw::NMusic
