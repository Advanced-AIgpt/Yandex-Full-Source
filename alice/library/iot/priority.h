#pragma once

#include "indexer.h"
#include "structs.h"

#include <library/cpp/scheme/scheme.h>


namespace NAlice::NIot {

using TPriorityType = decltype(NSc::Null().GetIntNumber());
static_assert(std::is_signed<TPriorityType>::value);

constexpr TPriorityType DEFAULT_PRIORITY = 0;
constexpr TPriorityType NO_MATCHED_INSTANCE_PRIORITY_DECREASE = 50;
constexpr TPriorityType EVERYWHERE_ROOM_PRIORITY_DECREASE = 4000;

void ChangePriority(const TPriorityType delta, NSc::TValue& hypothesis);

TPriorityType RawEntitiesCoverageToHypothesisPriorityChange(const TParsingHypothesis& ph);

TPriorityType CalculatePriorityInMultiSmartHomeEnvironment(const NSc::TValue& hypothesis, const TSmartHomeIndex& shIndex);

void AddBonusForCorrectPrepositionBeforeDatetimeRange(const TParsingHypothesis& ph, NSc::TValue& preliminaryHypothesis);

void AddBonus(const TParsingHypothesis& ph, NSc::TValue& hypothesis);

} // namespace NAlice::NIot
