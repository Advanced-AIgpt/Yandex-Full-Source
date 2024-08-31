#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioSceneElariWatch : public TScene<TMusicScenarioSceneArgsElariWatch> {
public:
    TMusicScenarioSceneElariWatch(const TScenario* owner);
    TRetMain Main(const TMusicScenarioSceneArgsElariWatch&, const TRunRequest&, TStorage&, const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
