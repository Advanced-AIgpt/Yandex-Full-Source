#pragma once

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/frame/slot.h>

#include <alice/library/json/json.h>
#include <alice/library/sys_datetime/sys_datetime.h>

#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/datetime/base.h>
#include <util/draft/date.h>

namespace NAlice::NHollywood {

// Дата не актуальна, если она в прошлом более, чем на неделю.
// default = true
inline bool IsActual(const TMaybe<TSysDatetimeParser>& sysdatetime, const TInstant& localTime, int maxDaysBefore = 7) {
    if (!sysdatetime) {
        return true;
    }
    NDatetime::TCivilSecond currentTime = NDatetime::Convert(localTime, NDatetime::GetLocalTimeZone());
    NDatetime::TCivilSecond resultTime = sysdatetime->GetTargetDateTime(currentTime, TSysDatetimeParser::TenseDefault);
    return NDatetime::GetCivilDiff(currentTime, resultTime, NDatetime::ECivilUnit::Day).Value < maxDaysBefore;
}

} // namespace NAlice::NHollywood
