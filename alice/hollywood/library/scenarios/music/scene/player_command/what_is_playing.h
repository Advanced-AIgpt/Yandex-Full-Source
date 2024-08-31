#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandWhatIsPlaying : public TScene<TMusicScenarioSceneArgsPlayerCommandWhatIsPlaying> {
public:
    TMusicScenarioScenePlayerCommandWhatIsPlaying(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandWhatIsPlaying&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
