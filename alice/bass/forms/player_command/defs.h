#pragma once

#include <alice/bass/forms/player_command/player_command.sc.h>

#include <alice/bass/forms/video/defs.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/util/scheme_holder.h>

namespace NBASS {
namespace NPlayerCommand {

constexpr TStringBuf IRRELEVANT_PLAYER_CODE = "irrelevant_player_command";

using TPlayerRewindCommand = TSchemeHolder<NBassApi::TPlayerRewindCommand<TSchemeTraits>>;
using TLastWatchedVideo = TMaybe<NVideo::TWatchedVideoItemScheme>;

enum class ERewindType {
    None /* "" */,
    Forward /* "forward" */,
    Backward /* "backward" */,
    Absolute /* "absolute" */
};

} // namespace NPlayerCommand
} // namespace NBASS
