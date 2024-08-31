#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandWhatYearIsThisSong : public TScene<TMusicScenarioSceneArgsPlayerCommandWhatYearIsThisSong> {
public:
    TMusicScenarioScenePlayerCommandWhatYearIsThisSong(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandWhatYearIsThisSong&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
