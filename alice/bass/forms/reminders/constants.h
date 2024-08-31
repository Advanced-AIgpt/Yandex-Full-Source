#pragma once

#include <util/generic/strbuf.h>

namespace NBASS::NReminders {

inline constexpr TStringBuf REMINDERS_FORM_NAME_CANCEL = "personal_assistant.scenarios.create_reminder__cancel";
inline constexpr TStringBuf REMINDERS_FORM_NAME_LIST_CANCEL = "personal_assistant.scenarios.list_reminders";
inline constexpr TStringBuf REMINDERS_FORM_NAME_LIST_CANCEL_RESETTING = "personal_assistant.scenarios.list_reminders__scroll_reset";
inline constexpr TStringBuf REMINDERS_FORM_NAME_PUSH_LANDING = "personal_assistant.scenarios.alarm_reminder";
inline constexpr TStringBuf SLOT_NAME_DATE = "date";
inline constexpr TStringBuf SLOT_NAME_DAY_PART = "day_part";
inline constexpr TStringBuf SLOT_NAME_TIME = "time";
inline constexpr TStringBuf SLOT_NAME_WHAT = "what";
inline constexpr TStringBuf SLOT_TYPE_DATE = "date";
inline constexpr TStringBuf SLOT_TYPE_DAY_PART = "day_part";
inline constexpr TStringBuf SLOT_TYPE_WHAT = "string";

} // namespace NBASS::NReminders
