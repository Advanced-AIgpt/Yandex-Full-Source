#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NMusic {

TMaybe<TMusicScenarioSceneArgsMultiroomRedirect> TryCreateMultiroomRedirectSceneArgs(
    const TScenarioRequestData& requestData,
    const google::protobuf::Message& tsfProto,
    bool addPlayerFeature = false,
    bool onlyToMasterDevice = false);

class TMusicScenarioSceneMultiroomRedirect : public TScene<TMusicScenarioSceneArgsMultiroomRedirect> {
public:
    TMusicScenarioSceneMultiroomRedirect(const TScenario* owner);
    TRetMain Main(const TMusicScenarioSceneArgsMultiroomRedirect&, const TRunRequest&, TStorage&, const TSource&) const override;
};

} // namespace NAlice::NHollywoodFw::NMusic
