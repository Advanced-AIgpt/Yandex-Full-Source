#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/library/scenarios/alarm/date_time.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>


#include <util/datetime/base.h>
#include <util/generic/strbuf.h>

namespace NBASS::NReminders {

inline constexpr TStringBuf SLOT_TYPE_TIME = "time";

bool AreTimersAndAlarmsEnabled(const TContext& ctx);

void CreateHowLongSlot(TContext& ctx, const NAlice::NScenarios::NAlarm::TDayTime& time);

TInstant GetCurrentTimestamp(const TContext& ctx);

bool CreateDateTime(TStringBuf epochStr, NDatetime::TCivilSecond now, NDatetime::TTimeZone tzNow, NSc::TValue& to);
bool CreateDateTime(TInstant epoch, NDatetime::TCivilSecond now, NDatetime::TTimeZone tzNow, NSc::TValue& to);

bool ConvertFromISO8601(TStringBuf rd, NDatetime::TCivilSecond now, NDatetime::TTimeZone tzNow, NSc::TValue* to);

NSc::TValue MakeScledTimeDirective(const NDatetime::TCivilSecond& time, const NDatetime::TCivilSecond& currTime);

NSc::TValue MakeScledTimeDirective(const TDuration& time, const NDatetime::TCivilSecond& currTime);

} // namespace NBASS::NReminders
