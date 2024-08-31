#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

NHollywood::TMusicArguments MakeMusicArguments(const TRunRequest& request, bool isNewContentRequestedByUser);

} // namespace NAlice::NHollywoodFw::NMusic
