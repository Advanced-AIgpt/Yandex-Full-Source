#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandRemoveDislike : public TScene<TMusicScenarioSceneArgsPlayerCommandRemoveDislike> {
public:
    TMusicScenarioScenePlayerCommandRemoveDislike(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandRemoveDislike&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;

    TRetSetup CommitSetup(
        const TMusicScenarioSceneArgsPlayerCommandRemoveDislike&,
        const TCommitRequest&,
        const TStorage&) const override;

    TRetCommit Commit(
        const TMusicScenarioSceneArgsPlayerCommandRemoveDislike&,
        const TCommitRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
