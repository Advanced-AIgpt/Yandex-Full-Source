#pragma once

#include <float.h>

namespace NAlice::NMegamind {

constexpr double MM_COMMANDS_PRIORITY = 0.8;
constexpr double MM_POST_CLASSIFY_PRIORITY = 0.6;
constexpr double MM_PROTOCOL_SCENARIO_PRIORITY = 0.7;
constexpr double MM_BOOSTED_VINS_SCENARIO_PRIORITY = MM_PROTOCOL_SCENARIO_PRIORITY + DBL_EPSILON;
constexpr double MM_SWAP_TRICK_PRIORITY = 0.5; // must be less than VINS's priority

constexpr double MM_DISABLED_SCENARIO_PRIORITY = -1.0;

} // namespace NAlice::NMegamind
