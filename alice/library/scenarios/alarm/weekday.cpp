#include "weekday.h"

#include <util/generic/algorithm.h>
#include <util/generic/array_size.h>

namespace NAlice::NScenarios::NAlarm {

namespace {
constexpr int NUM_WEEKDAYS = 7;
constexpr NDatetime::TWeekday WEEKDAYS[] = {NDatetime::TWeekday::monday,    NDatetime::TWeekday::tuesday,
                                            NDatetime::TWeekday::wednesday, NDatetime::TWeekday::thursday,
                                            NDatetime::TWeekday::friday,    NDatetime::TWeekday::saturday,
                                            NDatetime::TWeekday::sunday};
static_assert(Y_ARRAY_SIZE(WEEKDAYS) == NUM_WEEKDAYS, "");
}

bool ParseWeekday(const NSc::TValue& value, NDatetime::TWeekday& weekday) {
    if (!value.IsIntNumber()) {
        return false;
    }

    i64 wd = value.GetIntNumber();

    if (wd < 1 || wd > static_cast<i64>(Y_ARRAY_SIZE(WEEKDAYS))) {
        return false;
    }

    --wd;

    Y_ASSERT(wd >= 0);
    Y_ASSERT(static_cast<size_t>(wd) < Y_ARRAY_SIZE(WEEKDAYS));

    weekday = WEEKDAYS[wd];

    return true;
}

size_t GetWeekdayIndex(NDatetime::TWeekday weekday) {
    const auto index = FindIndex(WEEKDAYS, weekday);
    Y_ASSERT(index != NPOS);
    return index;
}

} // namespace NAlice::NScenarios::NAlarm
