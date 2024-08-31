#include "weekdays.h"
#include "weekday.h"

#include <util/generic/algorithm.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

namespace NAlice::NScenarios::NAlarm {

namespace {

TMaybe<TVector<NDatetime::TWeekday>> ParseWeekdays(const NSc::TValue& values) {
    if (!values.IsArray()) {
        return Nothing();
    }

    TVector<NDatetime::TWeekday> weekdays;
    for (const auto& value : values.GetArray()) {
        NDatetime::TWeekday weekday;
        if (!ParseWeekday(value, weekday)) {
            return Nothing();
        }
        weekdays.push_back(weekday);
    }

    if (weekdays.empty()) {
        return Nothing();
    }

    SortUnique(weekdays);
    return weekdays;
}

TMaybe<bool> ParseRepeat(const NSc::TValue& value) {
    if (value.IsNull())
        return MakeMaybe<bool>(false);

    if (!value.IsBool())
        return Nothing();

    return value.GetBool();
}
} // namespace

// static
TMaybe<TWeekdays> TWeekdays::FromValue(const NSc::TValue& value) {
    const auto weekdays = ParseWeekdays(value[WEEKDAYS_KEY_WEEKDAYS]);
    if (!weekdays) {
        return Nothing();
    }

    const auto repeat = ParseRepeat(value[WEEKDAYS_KEY_REPEAT]);
    if (!repeat) {
        return Nothing();
    }

    return TWeekdays(*weekdays, *repeat);
}

NSc::TValue TWeekdays::ToValue() const {
    NSc::TValue data;

    auto& weekdays = data[WEEKDAYS_KEY_WEEKDAYS].GetArrayMutable();
    for (const auto& day : Days)
        weekdays.push_back(GetWeekdayIndex(day) + 1);
    data[WEEKDAYS_KEY_REPEAT].SetBool(Repeat);

    return data;
}

bool TWeekdays::Contains(const NDatetime::TWeekday& weekday) const {
    return std::find(Days.begin(), Days.end(), weekday) != Days.end();
}

} // namespace NAlice::NScenarios::NAlarm

template <>
void Out<NAlice::NScenarios::NAlarm::TWeekdays>(IOutputStream& o, const NAlice::NScenarios::NAlarm::TWeekdays& weekdays) {
    o << "TWeekdays [";
    o << "[";
    bool first = true;
    for (const auto& day : weekdays.Days) {
        if (!first)
            o << " ";
        o << day;
        first = false;
    }
    o << "] ";
    o << weekdays.Repeat;
    o << "]";
}
