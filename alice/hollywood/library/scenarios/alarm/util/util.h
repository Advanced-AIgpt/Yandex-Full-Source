#pragma once

#include <alice/hollywood/library/scenarios/alarm/context/context.h>
#include <alice/hollywood/library/music/music_resources.h>
#include <alice/hollywood/library/util/tptrwrapper.h>

#include <alice/library/scenarios/alarm/helpers.h>

#include <alice/library/websearch/prepare_search_request.h>

#include <util/string/cast.h>

namespace NAlice::NHollywood::NReminders {

namespace NNlgTemplateNames {

constexpr TStringBuf SUGGESTS = "suggests";

constexpr TStringBuf ALARM_CANCEL = "alarm_cancel";
constexpr TStringBuf ALARM_HOW_LONG = "alarm_how_long";
constexpr TStringBuf ALARM_HOW_TO_SET_SOUND = "alarm_how_to_set_sound";
constexpr TStringBuf ALARM_MORNING_SHOW_ERROR = "alarm_morning_show_error";
constexpr TStringBuf ALARM_PLAY_ALICE_SHOW = "alarm_play_alice_show";
constexpr TStringBuf ALARM_PLAY_MORNING_SHOW = "alarm_play_morning_show";
constexpr TStringBuf ALARM_RESET_SOUND = "alarm_reset_sound";
constexpr TStringBuf ALARM_SET = "alarm_set";
constexpr TStringBuf ALARM_SET_ALICE_SHOW = "alarm_set_alice_show";
constexpr TStringBuf ALARM_SET_MORNING_SHOW = "alarm_set_morning_show";
constexpr TStringBuf ALARM_SET_SOUND = "alarm_set_sound";
constexpr TStringBuf ALARM_SET_WITH_ALICE_SHOW = "alarm_set_with_alice_show";
constexpr TStringBuf ALARM_SET_WITH_MORNING_SHOW = "alarm_set_with_morning_show";
constexpr TStringBuf ALARM_SET_WITH_SOUND = "alarm_set_with_sound";
constexpr TStringBuf ALARM_SHOW = "alarm_show";
constexpr TStringBuf ALARM_STOP_PLAYING = "alarm_stop_playing";
constexpr TStringBuf ALARM_SOUND_SET_LEVEL = "alarm_sound_set_level";
constexpr TStringBuf ALARM_WHAT_SOUND_IS_SET = "alarm_what_sound_is_set";
constexpr TStringBuf ALARM_WHAT_SOUND_LEVEL_IS_SET = "alarm_what_sound_level_is_set";

constexpr TStringBuf ALARM_FALLBACK = "alarm_fallback";

constexpr TStringBuf TIMER_CANCEL = "timer_cancel";
constexpr TStringBuf TIMER_HOW_LONG = "timer_how_long";
constexpr TStringBuf TIMER_PAUSE = "timer_pause";
constexpr TStringBuf TIMER_RESUME = "timer_resume";
constexpr TStringBuf TIMER_SET = "timer_set";
constexpr TStringBuf TIMER_SHOW = "timer_show";

constexpr TStringBuf ALREADY_ACTIONED = "already_actioned";
constexpr TStringBuf ALREADY_PAUSED = "already_paused";
constexpr TStringBuf ALREADY_PLAYING = "already_playing";
constexpr TStringBuf ASK_SOUND_LEVEL = "ask__sound_level";
constexpr TStringBuf ASK_TIME = "ask__time";
constexpr TStringBuf ASK_POSSIBLE_ALARMS = "ask__possible_alarms";
constexpr TStringBuf ASK_CORRECTION_DAY_PART = "ask__correction_day_part";
constexpr TStringBuf ERROR = "error";
constexpr TStringBuf NOT_SUPPORTED = "not_supported";
constexpr TStringBuf NO_ALARMS = "no_alarms";
constexpr TStringBuf NO_ALARMS_IN_NEAREST_FUTURE = "no_alarms_in_nearest_future";
constexpr TStringBuf NO_TIMERS = "no_timers";
constexpr TStringBuf BAD_ARGUMENTS = "bad_arguments";
constexpr TStringBuf TOO_MANY_ALARMS = "too_many_alarms";
constexpr TStringBuf TOO_MANY_TIMERS = "too_many_timers";
constexpr TStringBuf INVALID_ID = "invalid_id";
constexpr TStringBuf INVALID_TIME_ZONE = "invalid_time_zone";
constexpr TStringBuf RENDER_RESULT = "render_result";
constexpr TStringBuf RENDER_SOUND_ERROR = "render_error__sounderror";
constexpr TStringBuf SETTING_FAILED = "setting_failed";

} // namespace NNlgTemplateNames

namespace NFrameNames {

constexpr TStringBuf ALARM_ASK_SOUND = "personal_assistant.scenarios.alarm_ask_sound";
constexpr TStringBuf ALARM_ASK_TIME = "personal_assistant.scenarios.alarm_ask_time";
constexpr TStringBuf ALARM_CANCEL = "personal_assistant.scenarios.alarm_cancel";
constexpr TStringBuf ALARM_CANCEL__ELLIPSIS = "personal_assistant.scenarios.alarm_cancel__ellipsis";
constexpr TStringBuf ALARM_HOW_LONG = "personal_assistant.scenarios.alarm_how_long";
constexpr TStringBuf ALARM_HOW_TO_SET_SOUND = "personal_assistant.scenarios.alarm_how_to_set_sound";
constexpr TStringBuf ALARM_PLAY_MORNING_SHOW = "personal_assistant.scenarios.alarm_play_morning_show";
constexpr TStringBuf ALARM_RESET_SOUND = "personal_assistant.scenarios.alarm_reset_sound";
constexpr TStringBuf ALARM_SET = "personal_assistant.scenarios.alarm_set";
constexpr TStringBuf ALARM_SET_ALICE_SHOW = "personal_assistant.scenarios.alarm_set_alice_show";
constexpr TStringBuf ALARM_SET_MORNING_SHOW = "personal_assistant.scenarios.alarm_set_morning_show";
constexpr TStringBuf ALARM_SET_SOUND = "personal_assistant.scenarios.alarm_set_sound";
constexpr TStringBuf ALARM_SET_WITH_ALICE_SHOW = "personal_assistant.scenarios.alarm_set_with_alice_show";
constexpr TStringBuf ALARM_SET_WITH_MORNING_SHOW = "personal_assistant.scenarios.alarm_set_with_morning_show";
constexpr TStringBuf ALARM_SET_WITH_SOUND = "personal_assistant.scenarios.alarm_set_with_sound";
constexpr TStringBuf ALARM_SHOW = "personal_assistant.scenarios.alarm_show";
constexpr TStringBuf ALARM_SHOW__CANCEL = "personal_assistant.scenarios.alarm_show__cancel";
constexpr TStringBuf ALARM_SNOOZE_ABS = "personal_assistant.scenarios.alarm_snooze_abs";
constexpr TStringBuf ALARM_SNOOZE_REL = "personal_assistant.scenarios.alarm_snooze_rel";
constexpr TStringBuf ALARM_SOUND_SET_LEVEL = "personal_assistant.scenarios.alarm_sound_set_level";
constexpr TStringBuf ALARM_STOP_PLAYING = "personal_assistant.scenarios.alarm_stop_playing";
constexpr TStringBuf ALARM_WHAT_SOUND_IS_SET = "personal_assistant.scenarios.alarm_what_sound_is_set";
constexpr TStringBuf ALARM_WHAT_SOUND_LEVEL_IS_SET = "personal_assistant.scenarios.alarm_what_sound_level_is_set";

constexpr TStringBuf ALARM_FALLBACK = "personal_assistant.scenarios.alarm_fallback";

constexpr TStringBuf SLEEP_TIMER_CANCEL = "personal_assistant.scenarios.sleep_timer_cancel";
constexpr TStringBuf SLEEP_TIMER_SET = "personal_assistant.scenarios.sleep_timer_set";
constexpr TStringBuf SLEEP_TIMER_SET__ELLIPSIS = "personal_assistant.scenarios.sleep_timer_set__ellipsis";
constexpr TStringBuf SLEEP_TIMER_HOW_LONG = "personal_assistant.scenarios.sleep_timer_how_long";
constexpr TStringBuf TIMER_CANCEL = "personal_assistant.scenarios.timer_cancel";
constexpr TStringBuf TIMER_CANCEL__ELLIPSIS = "personal_assistant.scenarios.timer_cancel__ellipsis";
constexpr TStringBuf TIMER_HOW_LONG = "personal_assistant.scenarios.timer_how_long";
constexpr TStringBuf TIMER_PAUSE = "personal_assistant.scenarios.timer_pause";
constexpr TStringBuf TIMER_PAUSE__ELLIPSIS = "personal_assistant.scenarios.timer_pause__ellipsis";
constexpr TStringBuf TIMER_RESUME = "personal_assistant.scenarios.timer_resume";
constexpr TStringBuf TIMER_RESUME__ELLIPSIS = "personal_assistant.scenarios.timer_resume__ellipsis";
constexpr TStringBuf TIMER_SET = "personal_assistant.scenarios.timer_set";
constexpr TStringBuf TIMER_SET__ELLIPSIS = "personal_assistant.scenarios.timer_set__ellipsis";
constexpr TStringBuf TIMER_SHOW = "personal_assistant.scenarios.timer_show";
constexpr TStringBuf TIMER_STOP_PLAYING = "personal_assistant.scenarios.timer_stop_playing";

} // namespace NFrameNames

namespace NExperiments {

constexpr TStringBuf HW_ALARM_MORNING_SHOW = "hw_alarm_morning_show_exp";
constexpr TStringBuf HW_ALARM_SET_WITH_MORNING_SHOW = "hw_alarm_set_with_morning_show_exp";
constexpr TStringBuf HW_ALARM_RELOCATION__ALARM_SHOW_DISABLED = "hw_alarm_relocation_exp__alarm_show_disabled";
constexpr TStringBuf HW_ALARM_RELOCATION__ALARM_SET_SOUND = "hw_alarm_relocation_exp__alarm_set_sound";
constexpr TStringBuf HW_ALARM_RELOCATION__ALARM_SET_SOUND_RADIO = "hw_alarm_relocation_exp__alarm_set_sound_radio";

constexpr TStringBuf HW_ALARM_RELOCATION__FALLBACK = "hw_alarm_relocation_exp__fallback";

constexpr TStringBuf EXPERIMENTAL_FLAG_ALARMS_KEEP_OBSOLETE = "alarm_keep_obsolete";
constexpr TStringBuf CHANGE_ALARM_SOUND_DEBUG_FEATURE = "change_alarm_sound_debug_feature";

constexpr TStringBuf FLAG_ANALYTICS_MUSIC_WEB_RESPONSES = "analytics.music.add_web_responses";

constexpr TStringBuf NLG_SHORT_TIMER_PAUSE_EXP = "nlg_short_timer_pause_exp";
constexpr TStringBuf NLG_SHORT_TIMER_RESUME_EXP = "nlg_short_timer_resume_exp";
constexpr TStringBuf NLG_SHORT_TIMER_SHOW_EXP = "nlg_short_timer_show_exp";

const TVector<TStringBuf> NLG_EXPERIMENTS = {
    NLG_SHORT_TIMER_PAUSE_EXP,
    NLG_SHORT_TIMER_RESUME_EXP,
    NLG_SHORT_TIMER_SHOW_EXP
};

} // namespace NExperiment

namespace NScenarioNames {

constexpr TStringBuf ALARM = "alarm";
constexpr TStringBuf MORNING_SHOW = "morning_show";
constexpr TStringBuf TIMER = "timer";
constexpr TStringBuf SLEEP_TIMER = "sleep_timer";

} // namespace NScenarioNames

namespace NSlots {

const TString SLOT_ALARM_ID = "alarm_id";
const TString SLOT_AVAILABLE_ALARMS = "available_alarms";
const TString SLOT_DATE = "date";
const TString SLOT_TIME = "time";
const TString SLOT_TYPEPARSER_TIME = "typeparser_time";
const TString SLOT_DAY_PART = "day_part";
const TString SLOT_POSSIBLE_ALARMS = "possible_alarms";
const TString SLOT_ALARM_SET_SUCCESS = "alarm_set_success";
const TString SLOT_REPEAT = "repeat";

constexpr TStringBuf SLOT_THIS = "this";
constexpr TStringBuf SLOT_MUSIC_SEARCH = "music_search";
constexpr TStringBuf SLOT_SEARCH_TEXT = "search_text";
constexpr TStringBuf SLOT_PLAYLIST = "playlist";
constexpr TStringBuf SLOT_FM_RADIO = "radio_search";
constexpr TStringBuf SLOT_FM_RADIO_FREQ = "radio_freq";
const TVector<TStringBuf> MUSIC_FILTER_SLOTS = {
    TStringBuf("genre"),
    TStringBuf("mood"),
    TStringBuf("activity"),
    TStringBuf("epoch"),
    TStringBuf("personality"),
    TStringBuf("special_playlist")
};


const TString SLOT_TIMER_ID = "timer_id";
const TString SLOT_AVAILABLE_TIMERS = "available_timers";
const TString SLOT_TIMER_SET_SUCCESS = "timer_set_success";
const TString SLOT_SLEEP_SPECIFICATION = "specification";

const TString TYPE_DATE = "sys.date";
const TString TYPE_LIST = "list";
const TString TYPE_NUM = "sys.num";
const TString TYPE_SELECTION_DEPRECATED = "sys.selection";
const TString TYPE_SELECTION = "custom.selection";
const TString TYPE_TIME = "sys.time";
const TString TYPE_TYPEPARSERTIME = "typeparser.time";
const TString TYPE_UNITSTIME = "sys.units_time";
const TString TYPE_WEEKDAYS = "sys.weekdays";
const TString TYPE_BOOL = "sys.bool";

}

const THashMap<TStringBuf, TStringBuf> FRAME_NAME_TO_EXP_FLAG = {
    {NFrameNames::ALARM_FALLBACK, NExperiments::HW_ALARM_RELOCATION__FALLBACK},
    {NFrameNames::ALARM_SET_SOUND, NExperiments::HW_ALARM_RELOCATION__ALARM_SET_SOUND},
};

bool CanProccessFrame(TRemindersContext& ctx, const TStringBuf frameName, bool checkExpFlagForSmartSpeaker = true);

constexpr TStringBuf ATTENTION_ALARM_SET_WITH_MORNING_SHOW_FALLBACK = "alarm__set_with_morning_show_fallback";
constexpr TStringBuf ATTENTION_ALARM_ALREADY_SET = "alarm__already_set";
constexpr TStringBuf ATTENTION_ALARM_SNOOZE = "alarm__snooze";
constexpr TStringBuf ATTENTION_ALARM_IS_ANDROID = "alarm__is_android";
constexpr TStringBuf ATTENTION_ALARM_MORNING_SHOW_NOT_SET = "alarm__morning_show_not_set";

constexpr TStringBuf ATTENTION_ALARM_ASK_TIME_FOR_DAY_PART = "alarm__ask_time_for_day_part";
constexpr TStringBuf ATTENTION_ALARM_NEED_CONFIRMATION = "alarm__need_confirmation";
constexpr TStringBuf ATTENTION_NO_ALARMS = "no_alarms";
constexpr TStringBuf ATTENTION_SOUND_IS_SUPPORTED = "alarm_sound__supported";
constexpr TStringBuf ATTENTION_PAYMENT_REQUIRED = "alarm_sound__payment_required";
constexpr TStringBuf ATTENTION_UNAUTHORIZED = "alarm_sound__unauthorized";
constexpr TStringBuf ATTENTION_ALARM_RADIO_SEARCH_FAILURE = "alarm_sound__radio_search_failure";

constexpr TStringBuf ATTENTION_UPDATE_TO_SUPPORT_SOUND = "alarm_sound__update_first";
constexpr TStringBuf ATTENTION_DEFAULT_SOUND_IS_SET = "alarm_sound__default_is_set";

constexpr TStringBuf ATTENTION_TIMER_ABS_TIME = "timer__abs_time";
constexpr TStringBuf ATTENTION_TIMER_IS_MOBILE = "timer__is_mobile";
constexpr TStringBuf ATTENTION_TIMER_NEED_CONFIRMATION = "timer__need_confirmation";
constexpr TStringBuf ATTENTION_MULTIPLE_TIMERS = "multiple_timers";

bool CheckExpFlag(
    TRemindersContext& ctx,
    const TStringBuf& flag,
    const TStringBuf& nlgTemplateName
);

TInstant GetCurrentTimestamp(TRemindersContext& ctx);

void ConstructResponse(TRemindersContext& ctx);

bool IsSlotEmpty(const TPtrWrapper<TSlot>& slot);

TPtrWrapper<TSlot> FindOrAddSlot(
    TFrame& frame,
    const TString& name,
    const TString& type
);

TPtrWrapper<TSlot> FindSlot(
    const TFrame& frame,
    const TString& name,
    const TString& type
);

TPtrWrapper<TSlot> AddSlot(
    TFrame& frame,
    const TSlot& slot
);

TPtrWrapper<TSlot> FindSingleSlot(
    const TFrame& frame,
    const TString& name
);

bool IsTodayOrTomorrow(
    const TInstant& now,
    const NDatetime::TTimeZone& tz,
    const TMaybe<NScenarios::NAlarm::TDate>& date
);

void AddAlarmSetCommand(
    TRemindersContext& ctx,
    const TInstant& now,
    const NDatetime::TTimeZone& tz,
    TVector<NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const NScenarios::NAlarm::TWeekdaysAlarm& alarm,
    const TFrame& frame,
    bool isSnooze = false
);

void RaiseErrorShowPromo(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplateName,
    const TStringBuf& errorCode,
    const TStringBuf& phraseName=NNlgTemplateNames::ERROR
);

void FillSlotsFromState(
    TFrame& frame,
    const TRemindersState& state
);

TStringBuf GetFrameName(
    const TRemindersState& state
);

constexpr TStringBuf UPDATE_FORM_CALLBACK = "update_form";

TMaybe<TFrame> TryGetCallbackUpdateFormFrame(
    const NScenarios::TCallbackDirective* callback
);

::google::protobuf::Struct ToPayloadUpdateForm(
    const TSemanticFrame& frame
);

TString ConstructedScledPattern(
    const NDatetime::TCivilSecond& time,
    const TString& pattern
);

bool IsFairyTaleFilterGenre(const TFrame& frame);

TVector<NAlice::TDeviceState::TTimers::TTimer> GetDeviceTimers(
    TRemindersContext& ctx,
    bool isSleepTimerRequest
);

} // namespace NAlice::NHollywood::NReminders
