#pragma once

#include <alice/protos/data/scenario/reminders/state.pb.h>

namespace NAlice {
class TRemindersOnShootSemanticFrame;

namespace NRemindersApi {
using TRemindersState = NData::NReminders::TState;
using TReminderProto = TRemindersState::TReminder;
using TReminderOnShootTsf = TRemindersOnShootSemanticFrame;

} // NRemindersApi
} // namespace NAlice
