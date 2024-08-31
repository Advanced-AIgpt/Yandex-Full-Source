#pragma once

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/maybe.h>

namespace NAlice::NScenarios::NAlarm {

bool ParseWeekday(const NSc::TValue& from, NDatetime::TWeekday& weekday);
size_t GetWeekdayIndex(NDatetime::TWeekday weekday);

} // namespace NAlice::NScenarios::NAlarm
