#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandShuffle : public TScene<TMusicScenarioSceneArgsPlayerCommandShuffle> {
public:
    TMusicScenarioScenePlayerCommandShuffle(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandShuffle&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
