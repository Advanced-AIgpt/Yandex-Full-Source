#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioSceneEqualizer : public TScene<TMusicScenarioSceneArgsEqualizer> {
public:
    TMusicScenarioSceneEqualizer(const TScenario* owner);
    TRetMain Main(const TMusicScenarioSceneArgsEqualizer&, const TRunRequest&, TStorage&, const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
