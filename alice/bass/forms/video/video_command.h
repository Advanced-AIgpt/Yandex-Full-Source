#pragma once

#include "defs.h"
#include "video.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/player_command/defs.h>

namespace NBASS::NVideo {

void AddAnalyticsInfoFromVideoCommand(TContext& ctx);

} // namespace NBASS::NVideo
