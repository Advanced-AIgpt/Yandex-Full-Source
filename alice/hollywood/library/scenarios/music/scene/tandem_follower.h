#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioSceneTandemFollower : public TScene<TMusicScenarioSceneArgsTandemFollower> {
public:
    TMusicScenarioSceneTandemFollower(const TScenario* owner);
    TRetMain Main(const TMusicScenarioSceneArgsTandemFollower&, const TRunRequest&, TStorage&, const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
