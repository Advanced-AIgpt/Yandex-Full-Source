#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

namespace NAlice::NHollywood::NMusic {

TMaybe<TString> TryGetS3AnimationPath(const TMusicArguments_EPlayerCommand playerCommand);

} // namespace NAlice::NHollywood::NMusic
