#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandRepeat : public TScene<TMusicScenarioSceneArgsPlayerCommandRepeat> {
public:
    TMusicScenarioScenePlayerCommandRepeat(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandRepeat&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
