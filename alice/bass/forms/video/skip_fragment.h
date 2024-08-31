#pragma once

#include "defs.h"
#include "player_command.h"
#include "video_command.h"
#include "video.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/player_command/defs.h>

namespace NBASS::NVideo {

IContinuation::TPtr SkipFragment(TContext& ctx);

bool IsPositionInsideTheFragment(
    double position,
    double start,
    double end
);

} // namespace NBASS::NVideo
