#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

namespace NAlice::NHollywoodFw::NMusic {

class TMusicScenario : public TScenario {
public:
    TMusicScenario();

private:
    TRetScene Dispatch(const TRunRequest&, const TStorage&, const TSource&) const;
    TMaybe<TRetScene> DispatchPlayerCommand(TScenarioRequestData&, TScenarioStateData&) const;
};

} // namespace NAlice::NHollywoodFw::NMusic
