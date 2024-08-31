#include "tandem_follower.h"

namespace NAlice::NHollywoodFw::NMusic {

TMusicScenarioSceneTandemFollower::TMusicScenarioSceneTandemFollower(const TScenario* owner)
    : TScene{owner, "tandem_follower"}
{
}

TRetMain TMusicScenarioSceneTandemFollower::Main(const TMusicScenarioSceneArgsTandemFollower& args, const TRunRequest&, TStorage&, const TSource&) const {
    return TReturnValueRender("music_play", "tandem_follower_not_supported", args);
}

} // namespace NAlice::NHollywoodFw::NMusic
