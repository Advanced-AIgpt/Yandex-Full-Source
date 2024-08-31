#include "registrator.h"

#include "alarm.h"
#include "reminder.h"
#include "timer.h"
#include "todo.h"

namespace NBASS {
namespace NReminders {

const TVector<TFormHandlerPair> FORM_HANDLER_PAIRS({
    /* Timer show */
    {"personal_assistant.scenarios.timer_show", TimerShow},
    /* Timer cancel */
    {"personal_assistant.scenarios.timer_show__cancel", TimerCancel},
    {"personal_assistant.scenarios.timer_cancel", TimerCancel},
    {"personal_assistant.scenarios.timer_cancel__ellipsis", TimerCancel},
    /* Timer set */
    {TIMER_FORM_NAME_SET, TimerSet},
    {"personal_assistant.scenarios.timer_set__ellipsis", TimerSet},
    /* Timer pause */
    {"personal_assistant.scenarios.timer_pause", TimerPause},
    {"personal_assistant.scenarios.timer_pause__ellipsis", TimerPause},
    {"personal_assistant.scenarios.timer_show__pause", TimerPause},
    /* Timer resume */
    {"personal_assistant.scenarios.timer_resume", TimerResume},
    {"personal_assistant.scenarios.timer_resume__ellipsis", TimerResume},
    {"personal_assistant.scenarios.timer_show__resume", TimerResume},
    /* Timer stop playing */
    {"personal_assistant.scenarios.timer_stop_playing", TimerStopPlaying},
    /* Timer time left */
    {"personal_assistant.scenarios.timer_how_long", TimerHowLong},
    /* Sleep timer set */
    {"personal_assistant.scenarios.sleep_timer_set", SleepTimerSet},
    {"personal_assistant.scenarios.sleep_timer_set__ellipsis", SleepTimerSet},
    /* Sleep timer time left */
    {"personal_assistant.scenarios.sleep_timer_how_long", SleepTimerHowLong},
    /* Alarm set */
    {"personal_assistant.scenarios.alarm_set", AlarmSet},
    {"personal_assistant.scenarios.alarm_set__ellipsis", AlarmSet},
    /* Alarm snooze */
    {"personal_assistant.scenarios.alarm_snooze_abs", AlarmSet},
    {"personal_assistant.scenarios.alarm_snooze_rel", AlarmSet},
    /* Alarm cancel */
    {"personal_assistant.scenarios.alarm_cancel", AlarmCancel},
    {"personal_assistant.scenarios.alarm_cancel__ellipsis", AlarmCancel},
    {"personal_assistant.scenarios.alarm_show__cancel", AlarmCancel},
    /* Alarm stop */
    {"personal_assistant.scenarios.alarm_stop_playing", AlarmStopPlaying},
    /* Alarms show */
    {"personal_assistant.scenarios.alarm_show", AlarmsShow},
    /* Alarm set sound */
    {"personal_assistant.scenarios.alarm_set_sound", AlarmSetSound},
    {"personal_assistant.scenarios.alarm_ask_sound", AlarmSetSound},
    /* Alarm how to set sound */
    {"personal_assistant.scenarios.alarm_how_to_set_sound", AlarmSetSound},
    /* Alarm reset sound */
    {"personal_assistant.scenarios.alarm_reset_sound", AlarmResetSound},
    /* Alarm what sound is set */
    {"personal_assistant.scenarios.alarm_what_sound_is_set", AlarmWhatSoundIsSet},
    {"personal_assistant.scenarios.alarm_how_long", AlarmHowLong},
    /* Alarm set with sound */
    {"personal_assistant.scenarios.alarm_set_with_sound", AlarmSetWithSound},
    {"personal_assistant.scenarios.alarm_set_with_sound__ellipsis", AlarmSetWithSound},
    /* Alarm sound set level */
    {"personal_assistant.scenarios.alarm_sound_set_level", AlarmSoundSetLevel},
    /* Alarm what sound level is set */
    {"personal_assistant.scenarios.alarm_what_sound_level_is_set", AlarmWhatSoundLevelIsSet},
    /* TODOs and reminders */
    {REMINDERS_FORM_NAME_PUSH_LANDING, ReminderLanding},
    {"personal_assistant.scenarios.create_reminder", ReminderCreate},
    {"personal_assistant.scenarios.create_reminder__ellipsis", ReminderCreate},
    {REMINDERS_FORM_NAME_CANCEL, ReminderCancel},
    {REMINDERS_FORM_NAME_LIST_CANCEL, RemindersListCancel},
    {"personal_assistant.scenarios.list_reminders__scroll_next", RemindersListCancel},
    {REMINDERS_FORM_NAME_LIST_CANCEL_RESETTING, RemindersListCancel},
    {"personal_assistant.scenarios.list_reminders__scroll_stop", RemindersListCancelStop},
    {TODO_FORM_NAME_CREATE, TodoCreate},
    {"personal_assistant.scenarios.create_todo__ellipsis", TodoCreate},
    {TODO_FORM_NAME_CANCEL, TodoCancelLast},
    {"personal_assistant.scenarios.cancel_todo", TodoCancel},
    {"personal_assistant.scenarios.cancel_todo__ellipsis", TodoCancel},
    {"personal_assistant.scenarios.list_todo", TodosList},
    {"personal_assistant.scenarios.list_todo__scroll_next", TodosList},
    {"personal_assistant.scenarios.list_todo__scroll_stop", TodosListStop}});

} // namespace NReminders
} // namespace NBASS
