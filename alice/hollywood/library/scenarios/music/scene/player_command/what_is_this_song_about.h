#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandWhatIsThisSongAbout : public TScene<TMusicScenarioSceneArgsPlayerCommandWhatIsThisSongAbout> {
public:
    TMusicScenarioScenePlayerCommandWhatIsThisSongAbout(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandWhatIsThisSongAbout&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;

    TRetResponse Render(
        const TMusicScenarioRenderArgsPlayerCommandWhatIsThisSongAbout&,
        TRender&) const;
};

} // namespace NAlice::NHollywoodFw::NMusic
