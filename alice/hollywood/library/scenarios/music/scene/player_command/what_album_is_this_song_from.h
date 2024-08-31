#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom : public TScene<TMusicScenarioSceneArgsPlayerCommandWhatAlbumIsThisSongFrom> {
public:
    TMusicScenarioScenePlayerCommandWhatAlbumIsThisSongFrom(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandWhatAlbumIsThisSongFrom&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
