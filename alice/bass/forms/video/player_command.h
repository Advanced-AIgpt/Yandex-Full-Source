#pragma once

#include "defs.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/player_command/defs.h>
#include <alice/bass/forms/player_command.h>

namespace NBASS::NVideo {

TResultValue RewindVideo(TContext& ctx, NPlayerCommand::TPlayerRewindCommand& command);

} // namespace NBASS::NVideo
