#pragma once

namespace NBASS {
class TContext;

namespace NReminders {
// Documentation:
// VinsBass: https://wiki.yandex-team.ru/assistant/dialogs/alarm/Vins-Bass-protokol/
// ServerActions: https://wiki.yandex-team.ru/quasar/alarms-protocol/
// Logic: https://wiki.yandex-team.ru/assistant/dialogs/alarm/
void AlarmSet(TContext& ctx);
void AlarmCancel(TContext& ctx);
void AlarmHowLong(TContext& ctx);
void AlarmStopPlaying(TContext& ctx);
void AlarmsShow(TContext& ctx);
void AlarmSetSound(TContext& ctx);
void AlarmResetSound(TContext& ctx);
void AlarmWhatSoundIsSet(TContext& ctx);
void AlarmSetWithSound(TContext& ctx);
void AlarmSoundSetLevel(TContext& ctx);
void AlarmWhatSoundLevelIsSet(TContext& ctx);
} // namespace NReminders
} // namespace NBASS
