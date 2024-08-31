#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NMusic {

TMusicArguments::EPlayerCommand FindPlayerCommand(const TScenarioBaseRequestWithInputWrapper& request);
const TPtrWrapper<NAlice::TSemanticFrame> FindPlayerFrame(const TScenarioBaseRequestWithInputWrapper& request);

} // namespace NAlice::NHollywood::NMusic
