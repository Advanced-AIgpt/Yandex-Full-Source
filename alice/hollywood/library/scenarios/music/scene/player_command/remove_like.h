#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayerCommandRemoveLike : public TScene<TMusicScenarioSceneArgsPlayerCommandRemoveLike> {
public:
    TMusicScenarioScenePlayerCommandRemoveLike(const TScenario* owner);

    TRetMain Main(
        const TMusicScenarioSceneArgsPlayerCommandRemoveLike&,
        const TRunRequest&,
        TStorage&,
        const TSource&) const override;

    TRetSetup CommitSetup(
        const TMusicScenarioSceneArgsPlayerCommandRemoveLike&,
        const TCommitRequest&,
        const TStorage&) const override;

    TRetCommit Commit(
        const TMusicScenarioSceneArgsPlayerCommandRemoveLike&,
        const TCommitRequest&,
        TStorage&,
        const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
