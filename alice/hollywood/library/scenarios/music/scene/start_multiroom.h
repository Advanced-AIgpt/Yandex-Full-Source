#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NMusic {

bool CanProcessStartMultiroom(const TRunRequest& request);

TMusicScenarioSceneArgsStartMultiroom BuildStartMultiroomSceneArgs(const TStartMultiroomSemanticFrame& frame);

class TMusicScenarioSceneStartMultiroom : public TScene<TMusicScenarioSceneArgsStartMultiroom> {
public:
    TMusicScenarioSceneStartMultiroom(const TScenario* owner);
    TRetMain Main(const TMusicScenarioSceneArgsStartMultiroom&, const TRunRequest&, TStorage&, const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
