#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenarioScenePlayLess : public TScene<TMusicScenarioSceneArgsPlayLess> {
public:
    TMusicScenarioScenePlayLess(const TScenario* owner);
    TRetMain Main(const TMusicScenarioSceneArgsPlayLess&, const TRunRequest&, TStorage&, const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
