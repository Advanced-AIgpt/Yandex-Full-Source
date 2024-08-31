#pragma once

#include "defs.h"
#include "player_command.h"
#include "video_command.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/player_command/defs.h>
#include <alice/bass/forms/video/video.h>

namespace NBASS::NVideo {

IContinuation::TPtr VideoHowLong(TContext& ctx);

} // namespace NBASS::NVideo
