#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandSongsByThisArtist : public TScene<TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist> {
public:
    TMusicScenarioScenePlayerCommandSongsByThisArtist(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;

    TRetSetup ContinueSetup(
        const TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist&,
        const TContinueRequest&,
        const TStorage&) const override;

    TRetContinue Continue(
        const TMusicScenarioSceneArgsPlayerCommandSongsByThisArtist&,
        const TContinueRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
