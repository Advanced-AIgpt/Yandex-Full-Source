#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandRewind : public TScene<TMusicScenarioSceneArgsPlayerCommandRewind> {
public:
    TMusicScenarioScenePlayerCommandRewind(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandRewind&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
