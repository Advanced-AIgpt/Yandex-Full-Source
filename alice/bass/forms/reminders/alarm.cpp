#include "alarm.h"

#include "helpers.h"

#include <alice/library/scenarios/alarm/date_time.h>
#include <alice/library/scenarios/alarm/helpers.h>

#include <alice/library/json/json.h>
#include <alice/library/music/catalog.h>

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/music/music.h>
#include <alice/bass/forms/music/providers.h>
#include <alice/bass/forms/player/player.h>
#include <alice/bass/forms/radio.h>
#include <alice/bass/forms/video/utils.h>

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/sound.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/radio/fmdb.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/calendar_parser/parser.h>

#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/string/builder.h>
#include <util/system/yassert.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <utility>

namespace NBASS {
namespace NReminders {

using NAlice::NScenarios::NAlarm::TDayTime;
using NAlice::NScenarios::NAlarm::TWeekdaysAlarm;
using NAlice::NScenarios::NAlarm::TWeekdays;
using NAlice::NScenarios::NAlarm::TDate;

using NAlice::NScenarios::NAlarm::TimeToValue;
using NAlice::NScenarios::NAlarm::DateToValue;

using NAlice::NScenarios::NAlarm::WEEKDAYS_KEY_REPEAT;
using NAlice::NScenarios::NAlarm::WEEKDAYS_KEY_WEEKDAYS;

void AddSetSoundCommand(TContext& ctx, const NSc::TValue& info, const NSc::TValue& data, const NSc::TValue& slotData, const bool forRadio = false);

void AddSetSoundCommandWithObject(TContext& ctx, const NSc::TValue& info, const NSc::TValue& object, const NSc::TValue& slotData, const bool forRadio = false);

namespace {

using TDeviceStateConst = NBASSRequest::TMetaConst<TSchemeTraits>::TDeviceStateConst;

constexpr TStringBuf ATTENTION_ALARM_ALREADY_SET = "alarm__already_set";
constexpr TStringBuf ATTENTION_ALARM_IS_ANDROID = "alarm__is_android";
constexpr TStringBuf ATTENTION_ALARM_NEED_CONFIRMATION = "alarm__need_confirmation";
constexpr TStringBuf ATTENTION_ASK_TIME_FOR_DAY_PART = "alarm__ask_time_for_day_part";
constexpr TStringBuf ATTENTION_INVALID_ID = "alarm__invalid_id";
constexpr TStringBuf ATTENTION_MULTIPLE_ALARMS = "alarm__multiple_alarms";
constexpr TStringBuf ATTENTION_NO_ALARMS_FOR_TIME = "alarm__no_alarms_for_time";
constexpr TStringBuf ATTENTION_NO_PLAYING_ALARM = "alarm__no_playing_alarm";
constexpr TStringBuf ATTENTION_SNOOZE = "alarm__snooze";
constexpr TStringBuf ATTENTION_ALARM_SET_SUCCESS = "alarm__success";
// for sound intents
constexpr TStringBuf ATTENTION_SOUND_IS_SUPPORTED = "alarm_sound__supported";
constexpr TStringBuf ATTENTION_UPDATE_TO_SUPPORT_SOUND = "alarm_sound__update_first";
constexpr TStringBuf ATTENTION_DEFAULT_SOUND_IS_SET = "alarm_sound__default_is_set";
constexpr TStringBuf ATTENTION_NO_ALARMS = "alarm_sound__no_alarms";
constexpr TStringBuf ATTENTION_MUSIC_FROM_RADIO = "alarm_sound__music_from_radio";
constexpr TStringBuf ATTENTION_MUSIC_FROM_VIDEO = "alarm_sound__music_from_video";
constexpr TStringBuf ATTENTION_SET_SONG_INSTEAD = "alarm_sound__set_song_instead";
constexpr TStringBuf ATTENTION_NOT_A_RADIO = "alarm_sound__not_a_radio";
constexpr TStringBuf ATTENTION_HOW_TO_SET_SOUND = "alarm_sound__how_to";
constexpr TStringBuf ATTENTION_REPEAT_SOUND = "alarm_sound__repeat";
constexpr TStringBuf ATTENTION_UNAUTHORIZED = "alarm_sound__unauthorized";
constexpr TStringBuf ATTENTION_NO_USER_INFO = "alarm_sound__no_user_info";
constexpr TStringBuf ATTENTION_PAYMENT_REQUIRED = "alarm_sound__payment_required";
constexpr TStringBuf ATTENTION_PLUS_PUSH = "alarm_sound__plus_push";
constexpr TStringBuf ATTENTION_UNKNOWN_SOUND_ERROR = "alarm_sound__unknown_error";
constexpr TStringBuf ATTENTION_RADIO_SEARCH_FAILURE = "alarm_sound__radio_search_failure";

constexpr TStringBuf ACTION_ALARM_NEW = "alarm_new";
constexpr TStringBuf ACTION_ALARMS_UPDATE = "alarms_update";
constexpr TStringBuf ACTION_ALARM_STOP = "alarm_stop";
constexpr TStringBuf ACTION_SHOW_ALARMS = "show_alarms";
// for sound intents
constexpr TStringBuf ACTION_ALARM_SET_SOUND = "alarm_set_sound";
constexpr TStringBuf ACTION_ALARM_RESET_SOUND = "alarm_reset_sound";

constexpr TStringBuf CODE_BAD_ARGUMENTS = "bad_arguments";
constexpr TStringBuf CODE_INVALID_TIME_FORMAT = "invalid_time_format";
constexpr TStringBuf CODE_INVALID_TIME_ZONE = "invalid_time_zone";
constexpr TStringBuf CODE_NO_ALARMS_AVAILABLE = "no_alarms_available";
constexpr TStringBuf CODE_NO_ALARMS_IN_NEAREST_FUTURE = "no_alarms_in_nearest_future";
constexpr TStringBuf CODE_SETTING_FAILED = "setting_failed";
constexpr TStringBuf CODE_TOO_MANY_ALARMS = "too_many_alarms";
constexpr TStringBuf CODE_UNSUPPORTED_OPERATION = "unsupported_operation";

constexpr TStringBuf SLOT_ALARM_ID = "alarm_id";
constexpr TStringBuf SLOT_ALARM_SET_SUCCESS = "confirmation";
constexpr TStringBuf SLOT_AVAILABLE_ALARMS = "available_alarms";
constexpr TStringBuf SLOT_DATE = "date";
constexpr TStringBuf SLOT_DAY_PART = "day_part";
constexpr TStringBuf SLOT_TIME = "time";
// for sound intents
constexpr TStringBuf SLOT_THIS = "this";
constexpr TStringBuf SLOT_TARGET = "target_type";
constexpr TStringBuf SLOT_MUSIC_SEARCH = "music_search";
constexpr TStringBuf SLOT_PLAYLIST = "playlist";
constexpr TStringBuf SLOT_RADIO_SEARCH = "radio_search";
constexpr TStringBuf SLOT_RADIO_FREQ = "radio_freq";
constexpr TStringBuf SLOT_REPEAT = "repeat";
constexpr TStringBuf SLOT_MUSIC_RESULT = "music_result";
constexpr TStringBuf SLOT_RADIO_RESULT = "radio_result";
constexpr TStringBuf SLOT_SOUND_LEVEL = "level";
const TVector<TStringBuf> MUSIC_FILTER_SLOTS = {
    TStringBuf("genre"),
    TStringBuf("mood"),
    TStringBuf("activity"),
    TStringBuf("epoch"),
    TStringBuf("personality"),
    TStringBuf("special_playlist")
};

constexpr TStringBuf TYPE_BOOL = "bool";
constexpr TStringBuf TYPE_DATE = "date";
constexpr TStringBuf TYPE_LIST = "list";
constexpr TStringBuf TYPE_NUM = "num";
constexpr TStringBuf TYPE_SELECTION = "selection";
constexpr TStringBuf TYPE_TIME = SLOT_TYPE_TIME;
constexpr TStringBuf TYPE_WEEKDAYS = "weekdays";
// for sound intents
constexpr TStringBuf TYPE_CURRENT = "current";
constexpr TStringBuf TARGET_TYPE_TRACK = "track";
constexpr TStringBuf TARGET_TYPE_RADIO = "radio";
constexpr TStringBuf TARGET_TYPE_ALBUM = "album";
constexpr TStringBuf TARGET_TYPE_ARTIST = "artist";
constexpr TStringBuf TARGET_TYPE_PLAYLIST = "playlist";

constexpr TStringBuf FORM_NAME_SNOOZE_ABS = "personal_assistant.scenarios.alarm_snooze_abs";
constexpr TStringBuf FORM_NAME_SNOOZE_REL = "personal_assistant.scenarios.alarm_snooze_rel";
constexpr TStringBuf FORM_NAME_HOW_TO_SET_SOUND = "personal_assistant.scenarios.alarm_how_to_set_sound";

constexpr TStringBuf SELECTION_ALL = "all";

constexpr int SUGGESTED_HOURS[] = {7, 6, 8};

constexpr size_t MAX_NUM_ALARMS = 25;

TMaybe<TString> GetAlarmsState(TContext& ctx) {
    const auto& deviceState = ctx.Meta().DeviceState();

    if (deviceState.HasAlarmsState()) {
        const auto& state = deviceState.AlarmsState();
        if (!state.HasICalendar())
            return Nothing();
        return ToString(state.ICalendar());
    }

    if (!deviceState.HasAlarmsStateObsolete())
        return Nothing();
    return ToString(deviceState.AlarmsStateObsolete());
}

// This class is just a container for commonly used fields.
struct TAlarmsContext {
    template <typename TAlarms>
    TAlarmsContext(const TInstant& now, const NDatetime::TTimeZone& timeZone, TAlarms&& alarms)
        : Now(now)
        , TimeZone(timeZone)
        , Alarms(std::forward<TAlarms>(alarms)) {
    }

    TInstant NowInstant() const {
        return Now;
    }

    NDatetime::TCivilSecond NowCivil() const {
        return NDatetime::Convert(Now, TimeZone);
    }

    TInstant Now;
    NDatetime::TTimeZone TimeZone;
    TVector<TWeekdaysAlarm> Alarms;
};

bool IsTodayOrTomorrow(const TInstant& now, const NDatetime::TTimeZone& tz, const TMaybe<TDate>& date) {
    return date && date->HasExactDay() ? date->IsTodayOrTomorrow(NDatetime::Convert(now, tz)) : false;
}

// An alarm that may be triggered in the future, with an index from
// the original alarms list.
struct TActiveAlarm {
    TActiveAlarm(const TWeekdaysAlarm& alarm, size_t index)
        : Alarm(alarm)
        , Index(index) {
    }

    bool operator<(const TActiveAlarm& rhs) const {
        return Alarm.Begin < rhs.Alarm.Begin;
    }

    NSc::TValue ToValue(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz) const {
        const auto begin = NDatetime::Convert(Alarm.Begin, tz);

        NSc::TValue value;
        value["id"] = Index;
        value["time"] = TimeToValue(begin);
        if (const TMaybe<TWeekdays> weekdays = Alarm.GetLocalWeekdays(tz))
            value["date"] = weekdays->ToValue();
        else
            value["date"] = DateToValue(now, begin);
        return value;
    }

    static TMaybe<size_t> IndexFromValue(const NSc::TValue& value) {
        if (!value.IsDict())
            return Nothing();

        if (!value.Has("id"))
            return Nothing();

        const i64 index = value["id"].GetIntNumber(-1);
        if (index < 0 || static_cast<ui64>(index) > std::numeric_limits<size_t>::max())
            return Nothing();

        return static_cast<size_t>(index);
    }

    static NSc::TValue ToValue(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz,
                               const TVector<TActiveAlarm>& alarms) {
        NSc::TValue value;
        value.SetArray();

        for (const auto& alarm : alarms)
            value.Push(alarm.ToValue(now, tz));

        return value;
    }

    static TMaybe<TVector<size_t>> IndicesFromValue(const NSc::TValue& value) {
        if (!value.IsArray())
            return Nothing();

        TVector<size_t> indices;
        for (const auto& item : value.GetArray()) {
            const TMaybe<size_t> index = IndexFromValue(item);
            if (!index)
                return Nothing();
            indices.push_back(*index);
        }

        return indices;
    }

    TWeekdaysAlarm Alarm;
    size_t Index = 0;
};

// Returns list of alarms that may be triggered.
TVector<TActiveAlarm> GetActiveAlarms(const TVector<TWeekdaysAlarm>& alarms, const TInstant& now) {
    TVector<TActiveAlarm> result;

    for (size_t i = 0; i < alarms.size(); ++i) {
        const auto alarm = alarms[i].GetRest(now);
        if (alarm)
            result.emplace_back(*alarm, i);
    }

    return result;
}

TVector<TWeekdaysAlarm> GetCalendarItems(const TContext& ctx, const NDatetime::TTimeZone& tz, TStringBuf data,
                                         TInstant now) {
    auto alarms = TWeekdaysAlarm::FromICalendar(tz, data);
    if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_ALARMS_KEEP_OBSOLETE))
        EraseIf(alarms, [now](const TWeekdaysAlarm& alarm) { return alarm.IsOutdated(now); });
    return alarms;
}

bool MatchesExactly(const TAlarmsContext& ac, const TWeekdaysAlarm& alarm, const TMaybe<TDayTime>& dayTime,
                    const TMaybe<TDate>& date) {
    return !alarm.IsRegular() && alarm.TriggersOnDayTime(dayTime, ac.NowCivil(), ac.TimeZone) &&
           alarm.TriggersOnlyOnDate(date, ac.NowCivil(), ac.TimeZone);
}

bool MatchesApproximately(const TAlarmsContext& ac, const TWeekdaysAlarm& alarm, const TMaybe<TDayTime>& dayTime,
                          const TMaybe<TDate>& date) {
    return alarm.TriggersOnDayTime(dayTime, ac.NowCivil(), ac.TimeZone) &&
           alarm.TriggersOnDate(date, ac.NowCivil(), ac.TimeZone);
}

bool MatchesExactly(const TAlarmsContext& ac, const TWeekdaysAlarm& alarm, const TMaybe<TDayTime>& dayTime,
                    const TWeekdays& weekdays) {
    return alarm.IsRegular() == weekdays.Repeat && alarm.TriggersOnDayTime(dayTime, ac.NowCivil(), ac.TimeZone) &&
           alarm.TriggersOnSameWeekdays(weekdays, ac.TimeZone);
}

bool MatchesApproximately(const TAlarmsContext& ac, const TWeekdaysAlarm& alarm, const TMaybe<TDayTime>& dayTime,
                          const TWeekdays& weekdays) {
    return alarm.TriggersOnDayTime(dayTime, ac.NowCivil(), ac.TimeZone) &&
           alarm.TriggersOnWeekdays(weekdays, ac.TimeZone);
}

bool MatchesApproximately(const TAlarmsContext& ac, const TWeekdaysAlarm& alarm, const TDayTime& dayTime) {
    return alarm.TriggersOnDayTime(dayTime, ac.NowCivil(), ac.TimeZone);
}

TVector<TActiveAlarm> FilterApproximateMatching(const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms,
                                                const TMaybe<TDayTime>& dayTime, const TMaybe<TDate>& date) {
    TVector<TActiveAlarm> matchingAlarms;

    for (const auto& activeAlarm : activeAlarms) {
        const auto& alarm = activeAlarm.Alarm;
        if (MatchesApproximately(ac, alarm, dayTime, date))
            matchingAlarms.push_back(activeAlarm);
    }

    return matchingAlarms;
}

TVector<TActiveAlarm> FilterApproximateMatching(const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms,
                                                const TMaybe<TDayTime>& dayTime, const TWeekdays& weekdays) {
    TVector<TActiveAlarm> matchingAlarms;

    for (const auto& activeAlarm : activeAlarms) {
        const auto& alarm = activeAlarm.Alarm;
        if (MatchesApproximately(ac, alarm, dayTime, weekdays))
            matchingAlarms.push_back(activeAlarm);
    }

    return matchingAlarms;
}

TVector<TActiveAlarm> FilterApproximateMatching(const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms,
                                                const TDayTime& dayTime) {
    TVector<TActiveAlarm> matchingAlarms;

    for (const auto& activeAlarm : activeAlarms) {
        const auto& alarm = activeAlarm.Alarm;
        if (MatchesApproximately(ac, alarm, dayTime))
            matchingAlarms.push_back(activeAlarm);
    }

    return matchingAlarms;
}

NSc::TValue MakeAlarmSetCallback(TContext& ctx, bool success) {
    TContext callback(ctx, ctx.FormName());

    callback.CopySlotsFrom(ctx, {SLOT_TIME, SLOT_DATE});

    TSlot* const successSlot = callback.CreateSlot(SLOT_ALARM_SET_SUCCESS, TYPE_BOOL /* type */, true /* optional */);
    Y_ASSERT(successSlot);
    successSlot->Value.SetBool(success);

    return callback.ToJson(TContext::EJsonOut::FormUpdate | TContext::EJsonOut::Resubmit);
}

void AddError(TContext& ctx, TStringBuf code) {
    ctx.AddErrorBlockWithCode(TError(TError::EType::ALARMERROR), code);
}

void AddAlarmSetSuggests(TContext& ctx) {
    if (ctx.MetaClientInfo().IsSmartSpeaker())
        return;

    for (const auto& hours : SUGGESTED_HOURS) {
        NSc::TValue payload;
        payload["time"]["hours"].SetIntNumber(hours);
        ctx.AddSuggest("alarm__set_alarm", std::move(payload));
    }
}

void AddAlarmShowSuggest(TContext& ctx) {
    if (ctx.MetaClientInfo().IsSmartSpeaker())
        return;

    NSc::TValue payload;
    payload["uri"].SetString(TClientActionUrl(TClientActionUrl::EType::ShowAlarms).ToString());
    ctx.AddSuggest("alarm__show_alarms", std::move(payload));
}

void AddAlarmSetCommand(TContext& ctx, const TInstant& now, const NDatetime::TTimeZone& tz,
                        TVector<TWeekdaysAlarm>& alarms, const TWeekdaysAlarm& alarm, const bool snooze = false) {
    ctx.CreateSlot(SLOT_TIME, TYPE_TIME, true /* optional */, TimeToValue(alarm.Begin, tz));

    if (!alarm.Weekdays) {
        const NDatetime::TCivilSecond curr = NDatetime::Convert(now, tz);
        const NDatetime::TCivilSecond then = NDatetime::Convert(alarm.Begin, tz);

        const TDate date(then);
        if (date.IsTodayOrTomorrow(curr)) {
            // For midnights, "tomorrow" is quite ambiguous, so it's
            // better to omit date specification at all.
            if (date.IsTomorrow(curr) && NAlice::NScenarios::NAlarm::IsMidnight(then))
                ctx.CreateSlot(SLOT_DATE, TYPE_DATE, true /* optional */, NSc::TValue{});
            else
                ctx.CreateSlot(SLOT_DATE, TYPE_DATE, true /* optional */, DateToValue(curr, date));
        }
    }

    for (const auto& existingAlarm : alarms) {
        if (alarm.IsSubsetOf(existingAlarm)) {
            ctx.AddAttention(ATTENTION_ALARM_ALREADY_SET);
            return;
        }
    }

    alarms.push_back(alarm);

    Y_ASSERT(ctx.ClientFeatures().SupportsAlarms());

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        if (snooze) {
            Y_ASSERT(alarms.size() == 1);
            ctx.AddCommand<TAlarmStopPlayingOnQuasarDirective>(ACTION_ALARM_STOP, {});
        }
        ctx.AddCommand<TAlarmUpdateDirective>(
            ACTION_ALARMS_UPDATE,
            TWeekdaysAlarm::ToICalendarPayload(alarms, true /* listeningIsPossible */),
            true /* beforeTts */
        );

        if (ctx.ClientFeatures().SupportsScledDisplay()) {
            ctx.AddCommand<TDrawScledAnimationsDirective>(
                "draw_scled_animations",
                MakeScledTimeDirective(NDatetime::Convert(alarm.Begin, tz), NDatetime::Convert(now, tz)),
                true /* beforeTts */
            );
        }
        ctx.AddStopListeningBlock();
        return;
    }

    NSc::TValue payload = TWeekdaysAlarm::ToICalendarPayload(alarms);

    payload["on_success"] = MakeAlarmSetCallback(ctx, true /* success */);
    payload["on_fail"] = MakeAlarmSetCallback(ctx, false /* success */);

    ctx.AddCommand<TAlarmNewDirective>(ACTION_ALARM_NEW, std::move(payload));
    ctx.AddAttention(ATTENTION_ALARM_NEED_CONFIRMATION);
}

void AddSingleShotAlarmCommand(TContext& ctx, const TInstant& now, const NDatetime::TTimeZone& tz,
                               TVector<TWeekdaysAlarm>& alarms, const TDayTime& dayTime, const bool snooze) {
    const TWeekdaysAlarm alarm = GetAlarmTime(now, tz, dayTime);
    return AddAlarmSetCommand(ctx, now, tz, alarms, alarm, snooze);
}

void AddWeekdaysAlarmCommand(TContext& ctx, const TInstant& now, const NDatetime::TTimeZone& tz,
                             TVector<TWeekdaysAlarm>& alarms, const TDayTime& dayTime, const NSc::TValue& weekdays) {
    const auto ws = TWeekdays::FromValue(weekdays);
    if (!ws) {
        LOG(ERR) << "Can't parse weekdays" << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    Y_ASSERT(ws->Repeat);

    const TWeekdaysAlarm alarm = GetAlarmWeekdays(now, tz, dayTime, *ws);
    return AddAlarmSetCommand(ctx, now, tz, alarms, alarm);
}

void AddDateAlarmCommand(TContext& ctx, const TInstant& now, const NDatetime::TTimeZone& tz,
                         TVector<TWeekdaysAlarm>& alarms, const TDayTime& dayTime, const NSc::TValue& date) {
    const auto d = TDate::FromValue(date);
    if (!d || !d->HasExactDay()) {
        LOG(ERR) << "Can't parse date" << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    const TMaybe<TWeekdaysAlarm> alarm = GetAlarmDateTime(now, tz, dayTime, *d);
    if (!alarm)
        return AddError(ctx, CODE_BAD_ARGUMENTS);

    return AddAlarmSetCommand(ctx, now, tz, alarms, *alarm);
}

template <typename TFn>
void WithAlarms(TContext& ctx, TFn&& fn) {
    const auto alarmsState = GetAlarmsState(ctx);
    if (!alarmsState)
        return AddError(ctx, CODE_NO_ALARMS_AVAILABLE);

    try {
        const TInstant now = GetCurrentTimestamp(ctx);
        const NDatetime::TTimeZone timeZone = NDatetime::GetTimeZone(ctx.UserTimeZone());
        TVector<TWeekdaysAlarm> alarms = GetCalendarItems(ctx, timeZone, *alarmsState, now);

        const TAlarmsContext ac{now, timeZone, std::move(alarms)};
        fn(ac);
    } catch (const NCalendarParser::TParser::TException& e) {
        LOG(ERR) << "Invalid alarms state: " << e << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    } catch (const NDatetime::TInvalidTimezone& e) {
        LOG(ERR) << "Invalid time zone: " << ctx.UserTimeZone() << " " << e.what() << Endl;
        return AddError(ctx, CODE_INVALID_TIME_ZONE);
    }
}

// When |slot| is empty, sets result to Nothing, returns true.
// Otherwise, tries to parse slot value, returns true on success,
// otherwise logs error message and returns false.
template <typename T>
bool FromSlotImpl(const TSlot* slot, TMaybe<T>& result) {
    if (IsSlotEmpty(slot)) {
        result.Clear();
        return true;
    }

    const TMaybe<T> r = T::FromValue(slot->Value);
    if (!r) {
        LOG(ERR) << "Can't parse " << slot->Name << Endl;
        return false;
    }

    result = r;
    return true;
}

template <typename T>
bool FromSlot(const TSlot* slot, TMaybe<T>& result) {
    return FromSlotImpl(slot, result);
}

template <>
bool FromSlot(const TSlot* slot, TMaybe<TDate>& result) {
    TMaybe<TDate> date;
    if (!FromSlotImpl(slot, date))
        return false;

    if (!date || date->HasExactDay()) {
        result = date;
        return true;
    }

    return false;
}

void AskAboutAlarm(TContext& ctx, const TAlarmsContext& ac, const TActiveAlarm& activeAlarm) {
    ctx.CreateSlot(SLOT_AVAILABLE_ALARMS, TYPE_LIST, true /* optional */,
                   TActiveAlarm::ToValue(ac.NowCivil(), ac.TimeZone, {activeAlarm}));
    ctx.CreateSlot(SLOT_ALARM_ID, TYPE_NUM, true /* optional */, NSc::TValue().SetIntNumber(1));
    ctx.AddAttention(ATTENTION_NO_ALARMS_FOR_TIME);
}

void AskAboutAlarms(TContext& ctx, const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms) {
    if (activeAlarms.size() == 1)
        return AskAboutAlarm(ctx, ac, activeAlarms[0]);

    ctx.CreateSlot(SLOT_AVAILABLE_ALARMS, TYPE_LIST, true /* optional */,
                   TActiveAlarm::ToValue(ac.NowCivil(), ac.TimeZone, activeAlarms));
    ctx.AddAttention(ATTENTION_MULTIPLE_ALARMS);
}

void CancelAlarm(TContext& ctx, const TAlarmsContext& ac, size_t index) {
    auto alarms = ac.Alarms;

    if (index >= alarms.size()) {
        LOG(ERR) << "Invalid index: " << index << ", total number of alarms: " << alarms.size() << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    {
        const auto& alarm = alarms[index];
        const auto begin = NDatetime::Convert(alarm.Begin, ac.TimeZone);

        ctx.CreateSlot(SLOT_TIME, TYPE_TIME, true /* optional */, TimeToValue(begin));

        // Weekdays and date slots are mutually exclusive, only one of
        // them must be set.
        if (const TMaybe<TWeekdays> weekdays = alarm.GetLocalWeekdays(ac.TimeZone)) {
            ctx.CreateSlot(SLOT_DATE, TYPE_DATE, true /* optional */, NSc::TValue{});
            ctx.CreateSlot(SLOT_DATE, TYPE_WEEKDAYS, true /* optional */, weekdays->ToValue());
        } else {
            ctx.CreateSlot(SLOT_DATE, TYPE_WEEKDAYS, true /* optional */, NSc::TValue{});
            ctx.CreateSlot(SLOT_DATE, TYPE_DATE, true /* optional */, DateToValue(ac.NowCivil(), begin));
        }
    }

    alarms.erase(alarms.begin() + index);
    ctx.AddCommand<TAlarmOneCancelDirective>(
        ACTION_ALARMS_UPDATE,
        TWeekdaysAlarm::ToICalendarPayload(alarms),
        true /* beforeTts */
    );
}

void CancelAlarms(TContext& ctx, const TAlarmsContext& ac, TVector<size_t> indices) {
    SortUnique(indices);

    if (indices.size() == 1)
        return CancelAlarm(ctx, ac, indices[0]);

    TVector<TWeekdaysAlarm> result;
    try {
        result = RemoveByIndices(ac.Alarms, indices);
    } catch (const yexception& e) {
        LOG(ERR) << "Can't remove specified alarms: " << e << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    ctx.AddCommand<TAlarmManyCancelDirective>(
        ACTION_ALARMS_UPDATE,
        TWeekdaysAlarm::ToICalendarPayload(result),
        true /* beforeTts */
    );
}

void CancelAllAlarms(TContext& ctx) {
    ctx.AddCommand<TAlarmAllCancelDirective>(
        ACTION_ALARMS_UPDATE,
        TWeekdaysAlarm::ToICalendarPayload(TVector<TWeekdaysAlarm>{}),
        true /* beforeTts */
    );
}

// Cancel single alarm, one alarm already set.
void CancelSingle1Set(TContext& ctx, const TAlarmsContext& ac, const TActiveAlarm& activeAlarm,
                      const TMaybe<TDayTime>& dayTime, const TMaybe<TDate>& date) {
    const auto& alarm = activeAlarm.Alarm;

    if (!dayTime && !date)
        return CancelAlarm(ctx, ac, activeAlarm.Index);

    if (MatchesExactly(ac, alarm, dayTime, date))
        return CancelAlarm(ctx, ac, activeAlarm.Index);

    return AskAboutAlarm(ctx, ac, activeAlarm);
}

// Cancel single alarm, many alarms (> 1) are set.
void CancelSingleManySet(TContext& ctx, const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms,
                         const TMaybe<TDayTime>& dayTime, const TMaybe<TDate>& date) {
    Y_ASSERT(activeAlarms.size() > 1);

    if (!dayTime && !date)
        return AskAboutAlarms(ctx, ac, activeAlarms);

    TVector<TActiveAlarm> matchingAlarms;

    if (dayTime) {
        if (date)
            matchingAlarms = FilterApproximateMatching(ac, activeAlarms, *dayTime, date);
        else
            matchingAlarms = FilterApproximateMatching(ac, activeAlarms, *dayTime);
    } else {
        Y_ASSERT(date);
        matchingAlarms = FilterApproximateMatching(ac, activeAlarms, Nothing() /* dayTime */, *date);
    }

    if (matchingAlarms.empty())
        return AskAboutAlarms(ctx, ac, activeAlarms);

    if (matchingAlarms.size() == 1) {
        if (MatchesExactly(ac, matchingAlarms[0].Alarm, dayTime, date))
            return CancelAlarm(ctx, ac, matchingAlarms[0].Index);
        if (matchingAlarms[0].Alarm.IsRegular())
            return AskAboutAlarm(ctx, ac, matchingAlarms[0]);
    }

    return AskAboutAlarms(ctx, ac, matchingAlarms);
}

// Cancel regular alarm, one alarm already set (date is regular).
void CancelRegular1Set(TContext& ctx, const TAlarmsContext& ac, const TActiveAlarm& activeAlarm,
                       const TMaybe<TDayTime>& dayTime, const TWeekdays& weekdays) {
    if (MatchesExactly(ac, activeAlarm.Alarm, dayTime, weekdays))
        return CancelAlarm(ctx, ac, activeAlarm.Index);

    return AskAboutAlarm(ctx, ac, activeAlarm);
}

// Cancel regular alarm, many alarms (> 1) are set.
void CancelRegularManySet(TContext& ctx, const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms,
                          const TMaybe<TDayTime>& dayTime, const TWeekdays& weekdays) {
    Y_ASSERT(activeAlarms.size() > 1);

    const auto matchingAlarms = FilterApproximateMatching(ac, activeAlarms, dayTime, weekdays);

    if (matchingAlarms.empty())
        return AskAboutAlarms(ctx, ac, activeAlarms);

    if (matchingAlarms.size() == 1 && MatchesExactly(ac, matchingAlarms[0].Alarm, dayTime, weekdays))
        return CancelAlarm(ctx, ac, matchingAlarms[0].Index);

    return AskAboutAlarms(ctx, ac, matchingAlarms);
}

void CancelAllOnTimeAndDate(TContext& ctx, const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms,
                            const TMaybe<TDayTime>& dayTime, const TDate& date) {
    const auto matchingAlarms = FilterApproximateMatching(ac, activeAlarms, dayTime, date);

    if (matchingAlarms.empty())
        return AskAboutAlarms(ctx, ac, activeAlarms);

    bool allSingle = true;
    for (const auto& matchingAlarm : matchingAlarms) {
        if (!matchingAlarm.Alarm.TriggersOnlyOnDate(date, ac.NowCivil(), ac.TimeZone)) {
            allSingle = false;
            break;
        }
    }

    if (!allSingle)
        return AskAboutAlarms(ctx, ac, matchingAlarms);

    TVector<size_t> indices;
    for (const auto& matchingAlarm : matchingAlarms)
        indices.push_back(matchingAlarm.Index);
    return CancelAlarms(ctx, ac, indices);
}

void CancelById(TContext& ctx, const TAlarmsContext& ac, const NSc::TValue& alarmIdValue) {
    const auto* const availableAlarmsSlot = ctx.GetSlot(SLOT_AVAILABLE_ALARMS, TYPE_LIST);

    if (IsSlotEmpty(availableAlarmsSlot)) {
        LOG(ERR) << "Alarm id is specified but no " << SLOT_AVAILABLE_ALARMS << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    const TMaybe<TVector<size_t>> indices = TActiveAlarm::IndicesFromValue(availableAlarmsSlot->Value);
    if (!indices) {
        LOG(ERR) << "Can't parse list of available alarms" << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    i64 id = alarmIdValue.GetIntNumber(-1 /* default */);
    if (id < 0 || static_cast<ui64>(id) > indices->size()) {
        LOG(ERR) << "Invalid alarm id value: " << id << Endl;
        ctx.AddAttention(ATTENTION_INVALID_ID);
        return;
    }

    // Need to decrement here because alarmId is usually "first",
    // "second", etc., i.e. 1-based.
    if (id != 0)
        --id;

    Y_ASSERT(id >= 0 && static_cast<ui64>(id) < indices->size());
    return CancelAlarm(ctx, ac, (*indices)[id]);
}

void CancelBySelection(TContext& ctx, const TAlarmsContext& ac, const TVector<TActiveAlarm>& activeAlarms,
                       const NSc::TValue& alarmSelectionValue) {
    if (alarmSelectionValue.GetString("" /* default */) != SELECTION_ALL) {
        LOG(ERR) << "Invalid alarm selection value" << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    const auto* const availableAlarmsSlot = ctx.GetSlot(SLOT_AVAILABLE_ALARMS, TYPE_LIST);

    if (IsSlotEmpty(availableAlarmsSlot)) {
        const auto* const timeSlot = ctx.GetSlot(SLOT_TIME, TYPE_TIME);
        const auto* const dateSlot = ctx.GetSlot(SLOT_DATE, TYPE_DATE);
        const auto* const weekdaysSlot = ctx.GetSlot(SLOT_DATE, TYPE_WEEKDAYS);

        if (IsSlotEmpty(timeSlot) && IsSlotEmpty(dateSlot) && IsSlotEmpty(weekdaysSlot))
            return CancelAllAlarms(ctx);

        if (IsSlotEmpty(dateSlot) || !IsSlotEmpty(weekdaysSlot))
            return AddError(ctx, CODE_UNSUPPORTED_OPERATION);

        TMaybe<TDayTime> time;
        TMaybe<TDate> date;
        if (!FromSlot(timeSlot, time) || !FromSlot(dateSlot, date))
            return AddError(ctx, CODE_BAD_ARGUMENTS);

        Y_ASSERT(date);
        return CancelAllOnTimeAndDate(ctx, ac, activeAlarms, time, *date);
    }

    const TMaybe<TVector<size_t>> indices = TActiveAlarm::IndicesFromValue(availableAlarmsSlot->Value);
    if (!indices) {
        LOG(ERR) << "Can't parse list of available alarms" << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    return CancelAlarms(ctx, ac, *indices);
}

void AlarmCancelOnQuasar(TContext& ctx) {
    WithAlarms(ctx, [&ctx](const TAlarmsContext& ac) {
        const auto activeAlarms = GetActiveAlarms(ac.Alarms, ac.NowInstant());

        const auto* const alarmIdSlot = ctx.GetSlot(SLOT_ALARM_ID, TYPE_NUM);
        if (!IsSlotEmpty(alarmIdSlot))
            return CancelById(ctx, ac, alarmIdSlot->Value);

        const auto* const alarmSelectionSlot = ctx.GetSlot(SLOT_ALARM_ID, TYPE_SELECTION);
        if (!IsSlotEmpty(alarmSelectionSlot))
            return CancelBySelection(ctx, ac, activeAlarms, alarmSelectionSlot->Value);

        if (activeAlarms.empty())
            return AddError(ctx, CODE_NO_ALARMS_AVAILABLE);

        const auto* const timeSlot = ctx.GetSlot(SLOT_TIME, TYPE_TIME);
        const auto* const dateSlot = ctx.GetSlot(SLOT_DATE, TYPE_DATE);
        const auto* const weekdaysSlot = ctx.GetSlot(SLOT_DATE, TYPE_WEEKDAYS);

        if (!IsSlotEmpty(dateSlot) && !IsSlotEmpty(weekdaysSlot))
            return AddError(ctx, CODE_BAD_ARGUMENTS);

        TMaybe<TDayTime> time;
        TMaybe<TDate> date;
        TMaybe<TWeekdays> weekdays;
        if (!FromSlot(timeSlot, time) || !FromSlot(dateSlot, date) || !FromSlot(weekdaysSlot, weekdays))
            return AddError(ctx, CODE_BAD_ARGUMENTS);

        Y_ASSERT(!activeAlarms.empty());
        if (activeAlarms.size() == 1) {
            return weekdays ? CancelRegular1Set(ctx, ac, activeAlarms[0], time, *weekdays)
                            : CancelSingle1Set(ctx, ac, activeAlarms[0], time, date);
        }

        return weekdays ? CancelRegularManySet(ctx, ac, activeAlarms, time, *weekdays)
                        : CancelSingleManySet(ctx, ac, activeAlarms, time, date);
    });
}

void AlarmStopPlayingOnQuasar(TContext& ctx) {
    const auto& meta = ctx.Meta();

    if (meta.DeviceState().AlarmsState().CurrentlyPlaying())
        ctx.AddCommand<TAlarmStopPlayingOnQuasarDirective>(ACTION_ALARM_STOP, NSc::TValue{});
    else
        ctx.AddAttention(ATTENTION_NO_PLAYING_ALARM);
}

void AlarmsShowOnQuasar(TContext& ctx) {
    WithAlarms(ctx, [&ctx](const TAlarmsContext& ac) {
        auto activeAlarms = GetActiveAlarms(ac.Alarms, ac.Now);

        const auto* const timeSlot = ctx.GetSlot(SLOT_TIME, TYPE_TIME);
        const auto* const dateSlot = ctx.GetSlot(SLOT_DATE, TYPE_DATE);
        const auto* const weekdaysSlot = ctx.GetSlot(SLOT_DATE, TYPE_WEEKDAYS);

        TMaybe<TDayTime> time;
        TMaybe<TDate> date;
        TMaybe<TWeekdays> weekdays;

        if (!FromSlot(timeSlot, time) || !FromSlot(dateSlot, date) || !FromSlot(weekdaysSlot, weekdays))
            return AddError(ctx, CODE_BAD_ARGUMENTS);

        if (weekdays)
            activeAlarms = FilterApproximateMatching(ac, activeAlarms, time, *weekdays);
        else
            activeAlarms = FilterApproximateMatching(ac, activeAlarms, time, date);

        if (activeAlarms.empty())
            return AddError(ctx, CODE_NO_ALARMS_AVAILABLE);

        Sort(activeAlarms.begin(), activeAlarms.end());

        ctx.CreateSlot(SLOT_AVAILABLE_ALARMS, TYPE_LIST, true /* optional */,
                       TActiveAlarm::ToValue(ac.NowCivil(), ac.TimeZone, activeAlarms));
    });
}

bool IsEnabled(TContext& ctx) {
    if (ctx.ClientFeatures().SupportsAlarms()) {
        return true;
    }

    AddError(ctx, CODE_UNSUPPORTED_OPERATION);
    return false;
}

void TakeDayPartIntoAccount(TContext& ctx) {
    static const THashMap<TStringBuf, TStringBuf> dayPartToName = {
        {"night", "ночь"},
        {"nights", "ночь"},
        {"morning", "утро"},
        {"mornings", "утро"},
        {"day", "день"},
        {"evening", "вечер"},
        {"evenings", "вечер"}
    };

    TSlot* const dayPartSlot = ctx.GetSlot(SLOT_DAY_PART);
    if (IsSlotEmpty(dayPartSlot))
        return;

    TSlot* const timeSlot = ctx.GetSlot(SLOT_TIME);
    if (IsSlotEmpty(timeSlot)) {
        const auto* dayPartName = dayPartToName.FindPtr(dayPartSlot->Value.GetString());
        if (dayPartName && ctx.HasExpFlag("alarm_day_part")) {
            NSc::TValue data;
            data["day_part_name"] = *dayPartName;
            ctx.AddAttention(ATTENTION_ASK_TIME_FOR_DAY_PART, data);
        }
        return;
    }

    NAlice::NScenarios::NAlarm::AdjustTimeValue(timeSlot->Value, dayPartSlot->Value);

    if (NAlice::NScenarios::NAlarm::IsPluralDayPart(dayPartSlot->Value)) {
        TSlot* const dateSlot = ctx.GetSlot(SLOT_DATE, TYPE_DATE);
        TSlot* const weekdaysSlot = ctx.GetSlot(SLOT_DATE, TYPE_WEEKDAYS);
        if (!IsSlotEmpty(weekdaysSlot))
            weekdaysSlot->Value[WEEKDAYS_KEY_REPEAT].SetBool(true);
        else if (IsSlotEmpty(dateSlot)) {
            NSc::TValue value;
            value[WEEKDAYS_KEY_WEEKDAYS].SetArray().AppendAll({1, 2, 3, 4, 5, 6, 7});
            value[WEEKDAYS_KEY_REPEAT].SetBool(true);
            ctx.CreateSlot(SLOT_DATE, TYPE_WEEKDAYS, true /* optional */, value);
        }
    }
}

void SetAlarmProductScenario(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ALARM);
}

bool DoesMusicPlayerHaveTrackInfo(const TDeviceStateConst& state) {
    auto musicInfo = *state.Music().CurrentlyPlaying().TrackInfo().GetRawValue();
    return !musicInfo.IsNull();
}

void SetAlarmMusicPlayerTrack(TContext& ctx, const TDeviceStateConst::TMusicConst& musicPlayer,
                              const TStringBuf slotTargetValue)
{
    // Add a command and update slots if context and target are compatible
    if (slotTargetValue == TARGET_TYPE_RADIO) {
        // Music is not a radio
        ctx.AddAttention(ATTENTION_NOT_A_RADIO);
    } else {
        const auto musicInfo = *musicPlayer.CurrentlyPlaying().TrackInfo().GetRawValue();

        NSc::TValue result;
        NMusic::TYandexMusicAnswer answer(ctx.ClientFeatures());
        answer.InitWithShazamAnswer("track", musicInfo, /* autoplay */ false);
        answer.ConvertAnswerToOutputFormat(&result);

        NSc::TValue object;
        object["type"] = TStringBuf("track");
        object["id"] = result["id"].ForceString();

        AddSetSoundCommandWithObject(ctx, result, std::move(object), result);
    }
}

bool IsAudioPlayerPlaying(const TDeviceStateConst& state) {
    const auto& audioPlayer = state.AudioPlayer();
    return audioPlayer.PlayerState().GetRawValue()->ForceString() == "Playing";
}

bool DoesAudioPlayerHaveTrackInfo(const TDeviceStateConst& state) {
    const auto& audioPlayer = state.AudioPlayer();
    const auto& currentStream = audioPlayer.CurrentStream();
    const auto& trackId = currentStream.StreamId();
    const auto& trackTitle = currentStream.Title();
    const auto& artistName = currentStream.Subtitle();
    return !trackId->empty() && !trackTitle->empty() && !artistName->empty();
}

void SetAlarmAudioPlayerTrack(TContext& ctx, const TDeviceStateConst::TAudioPlayerConst& audioPlayer,
                              const TStringBuf slotTargetValue)
{
    // Add a command and update slots if context and target are compatible
    if (slotTargetValue == TARGET_TYPE_RADIO) {
        // Music is not a radio
        ctx.AddAttention(ATTENTION_NOT_A_RADIO);
    } else {
        // TODO(vitvlkv): Implement a better solution: take trackId from device state and then
        // get all the required track metainfo from the music backend
        // https://doc.yandex-team.ru/music/api-guide/concepts/track-info.html#track-info
        const auto& currentStream = audioPlayer.CurrentStream();
        const auto& trackId = currentStream.StreamId(); // XXX(vitvlkv): Yes, we rely here that
                                                                    // streamId == trackId, and this is not very good
        const auto& trackTitle = currentStream.Title();
        const auto& artistName = currentStream.Subtitle(); // Yes, subtitle == artist

        NSc::TValue result;
        result["type"] = TStringBuf("track");
        result["id"] = trackId;
        result["title"] = trackTitle;

        auto& artists = result["artists"].SetArray();
        NSc::TValue artistValue;
        artistValue["name"] = artistName;
        artists.Push() = artistValue;

        NSc::TValue object = result;

        AddSetSoundCommandWithObject(ctx, result, std::move(object), result);
    }
}

} // namespace

// VINS-BASS proto: https://wiki.yandex-team.ru/assistant/dialogs/alarm/Vins-Bass-protokol/#ustanovitbudilnik
// Server commands: https://wiki.yandex-team.ru/quasar/alarms-protocol/
void AlarmSet(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        ctx.AddOnboardingSuggest();
        return;
    }
    if (ctx.ClientFeatures().SupportsNoReliableSpeakers()) {
        AddError(ctx, CODE_UNSUPPORTED_OPERATION);
        ctx.AddOnboardingSuggest();
        return;
    }

    const bool snoozeIntent = ctx.FormName() == FORM_NAME_SNOOZE_ABS || ctx.FormName() == FORM_NAME_SNOOZE_REL;
    // Snooze events may actually come from alarm_set intent,
    // so this attention is not always accurate
    if (snoozeIntent || ctx.Meta().DeviceState().AlarmsState().CurrentlyPlaying()) {
        ctx.AddAttention(ATTENTION_SNOOZE);
    }

    const TInstant now = GetCurrentTimestamp(ctx);
    NDatetime::TTimeZone tz;
    try {
        tz = NDatetime::GetTimeZone(ctx.UserTimeZone());
    } catch (const NDatetime::TInvalidTimezone& e) {
        LOG(ERR) << "Invalid time zone: " << ctx.UserTimeZone() << " " << e.what() << Endl;
        return AddError(ctx, CODE_INVALID_TIME_ZONE);
    }

    TVector<TWeekdaysAlarm> alarms;
    try {
        const auto alarmsState = GetAlarmsState(ctx);
        if (ctx.MetaClientInfo().IsSmartSpeaker() && alarmsState) {
            alarms = GetCalendarItems(ctx, tz, *alarmsState, now);
        }
    } catch (const NCalendarParser::TParser::TException& e) {
        LOG(ERR) << "Invalid alarms state: " << e << Endl;
        return AddError(ctx, CODE_BAD_ARGUMENTS);
    }

    const TSlot* const successSlot = ctx.GetSlot(SLOT_ALARM_SET_SUCCESS);
    if (!IsSlotEmpty(successSlot)) {
        // This code is called when device responds via callback form.

        AddAlarmShowSuggest(ctx);

        if (!successSlot->Value.IsBool())
            return AddError(ctx, CODE_BAD_ARGUMENTS);

        const bool success = successSlot->Value.GetBool();
        if (!success)
            AddError(ctx, CODE_SETTING_FAILED);
        return;
    }

    if (GetActiveAlarms(alarms, now).size() >= MAX_NUM_ALARMS)
        return AddError(ctx, CODE_TOO_MANY_ALARMS);

    TakeDayPartIntoAccount(ctx);

    const auto* const timeSlot = ctx.GetSlot(SLOT_TIME);
    const auto* const dateSlot = ctx.GetSlot(SLOT_DATE, TYPE_DATE);
    auto* const weekdaysSlot = ctx.GetSlot(SLOT_DATE, TYPE_WEEKDAYS);

    // Only weekdays on repeat are supported
    if (!IsSlotEmpty(weekdaysSlot) && weekdaysSlot->Value.IsDict() &&
        weekdaysSlot->Value[WEEKDAYS_KEY_REPEAT].GetBool() == false
    ) {
        if (weekdaysSlot->Value[WEEKDAYS_KEY_WEEKDAYS].ArraySize() == 1) {
            // For cases like "в среду", "в воскресенье"
            return AddError(ctx, CODE_BAD_ARGUMENTS);
        }
        // For cases like "в выходные", "в рабочие дни"
        weekdaysSlot->Value[WEEKDAYS_KEY_REPEAT].SetBool(true);
    }

    if (IsSlotEmpty(timeSlot) && !snoozeIntent) {
        if (!IsSlotEmpty(dateSlot)) {
            const auto date = TDate::FromValue(dateSlot->Value);
            if (!IsTodayOrTomorrow(now, tz, date))
                return AddError(ctx, CODE_BAD_ARGUMENTS);
        }

        ctx.CreateSlot(SLOT_TIME, TYPE_TIME, false /* optional */, NSc::TValue{});

        AddAlarmShowSuggest(ctx);
        AddAlarmSetSuggests(ctx);

        return;
    }

    auto dayTime = !IsSlotEmpty(timeSlot)
        ? TDayTime::FromValue(timeSlot->Value)
        // default snooze time is 10 minutes
        : TDayTime(Nothing() /* hours */, TDayTime::TComponent(10, true /* relative */) /* minutes */,
                   Nothing() /* seconds */, TDayTime::EPeriod::Unspecified);
    if (!dayTime || dayTime->HasRelativeNegative()) {
        return AddError(ctx, CODE_INVALID_TIME_FORMAT);
    }

    // Snooze time is almost always relative
    if (ctx.FormName() == FORM_NAME_SNOOZE_REL && !dayTime->IsRelative() && dayTime->Period == TDayTime::EPeriod::Unspecified) {
        const auto setRelative = [&](auto& comp) {
            if (comp) {
                comp->Relative = true;
            }
        };

        // Create relative version of dayTime
        TDayTime relDayTime = *dayTime;
        setRelative(relDayTime.Hours);
        setRelative(relDayTime.Minutes);
        setRelative(relDayTime.Seconds);

        // We use relative dayTime if it happens sooner than absolute
        const auto realNow = NDatetime::Convert(now, tz);
        if (GetAlarmTime(realNow, relDayTime) < GetAlarmTime(realNow, *dayTime)) {
            dayTime = relDayTime;
        }
    }

    if (!IsSlotEmpty(dateSlot) && !IsSlotEmpty(weekdaysSlot))
        return AddError(ctx, CODE_BAD_ARGUMENTS);

    if (IsSlotEmpty(dateSlot) && IsSlotEmpty(weekdaysSlot))
        return AddSingleShotAlarmCommand(ctx, now, tz, alarms, *dayTime, snoozeIntent);

    if (!IsSlotEmpty(weekdaysSlot)) {
        Y_ASSERT(IsSlotEmpty(dateSlot));
        return AddWeekdaysAlarmCommand(ctx, now, tz, alarms, *dayTime, weekdaysSlot->Value);
    }

    if (!IsSlotEmpty(dateSlot)) {
        Y_ASSERT(IsSlotEmpty(weekdaysSlot));
        return AddDateAlarmCommand(ctx, now, tz, alarms, *dayTime, dateSlot->Value);
    }

    return AddError(ctx, CODE_UNSUPPORTED_OPERATION);
}

// VINS-BASS proto: https://wiki.yandex-team.ru/assistant/dialogs/alarm/Vins-Bass-protokol/#otmenitbudilnik
// Server commands: https://wiki.yandex-team.ru/quasar/alarms-protocol/
void AlarmCancel(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        return AlarmCancelOnQuasar(ctx);
    }

    AddAlarmShowSuggest(ctx);
    AddAlarmSetSuggests(ctx);

    ctx.AddCommand<TAlarmShowDirective>(ACTION_SHOW_ALARMS, NSc::TValue{});
    ctx.AddAttention(ATTENTION_ALARM_IS_ANDROID);
}

void AlarmHowLong(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    WithAlarms(ctx, [&ctx](const TAlarmsContext& ac) {
        auto activeAlarms = GetActiveAlarms(ac.Alarms, ac.Now);

        if (activeAlarms.empty())
            return AddError(ctx, CODE_NO_ALARMS_AVAILABLE);

        TDayTime answer(24, 60, 0, TDayTime::EPeriod::Unspecified);
        NDatetime::TCivilSecond conv = NDatetime::Convert(ac.Now, ac.TimeZone);
        TDayTime nowTime(conv.hour(), conv.minute(), 0, TDayTime::EPeriod::Unspecified);
        bool hasAlarmInNearestFuture = false;
        int nowDay = static_cast<int>(NAlice::NScenarios::NAlarm::GetWeekday(conv));
        for (auto al : activeAlarms) {
            auto beg = al.Alarm.Begin;
            conv = NDatetime::Convert(beg, ac.TimeZone);
            TDayTime alarmTime(conv.hour(), conv.minute(), 0, TDayTime::EPeriod::Unspecified);
            bool todayAlarm = IsFirstBeforeSecond(nowTime, alarmTime);
            if (al.Alarm.Weekdays) {
                for (auto d: al.Alarm.Weekdays->Days) {
                    int numDay = static_cast<int>(d);
                    int comp = (numDay + 7 - nowDay) % 7;
                    if (comp == 1 && !todayAlarm || comp == 0 && todayAlarm) {
                        hasAlarmInNearestFuture = true;
                    }
                }
            } else {
                hasAlarmInNearestFuture = true;
            }
            if (!hasAlarmInNearestFuture)
                continue;

            TDayTime diffTime;
            if (!todayAlarm)
                alarmTime.Hours->Value += 24;
            GetDistInTime(nowTime, alarmTime, diffTime);
            if (IsFirstBeforeSecond(diffTime, answer))
                answer = diffTime;
        }
        if (!hasAlarmInNearestFuture)
            return AddError(ctx, CODE_NO_ALARMS_IN_NEAREST_FUTURE);

        CreateHowLongSlot(ctx, answer);
    });
}

// VINS BASS proto:
// https://wiki.yandex-team.ru/assistant/dialogs/alarm/Vins-Bass-protokol/#vykljuchitigrajushhijjbudilnik
// Server commands: https://wiki.yandex-team.ru/quasar/alarms-protocol/
void AlarmStopPlaying(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        return AlarmStopPlayingOnQuasar(ctx);
    }

    const TSlot* const snoozeSlot = ctx.GetSlot("snooze" /* name */);
    const bool snooze = !IsSlotEmpty(snoozeSlot) && snoozeSlot->Value.GetBool();
    ctx.AddCommand<TAlarmStopPlayingDirective>(snooze ? ACTION_ALARM_STOP : ACTION_ALARMS_UPDATE, NSc::TValue{});
}

// VINS BASS proto:
// https://wiki.yandex-team.ru/assistant/dialogs/alarm/Vins-Bass-protokol/#posmotretspisokustanovlennyxbudilnikov
// Server commands: https://wiki.yandex-team.ru/quasar/alarms-protocol/
void AlarmsShow(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        AlarmsShowOnQuasar(ctx);
    } else {
        AddAlarmShowSuggest(ctx);
        AddAlarmSetSuggests(ctx);
        ctx.AddAttention(ATTENTION_ALARM_IS_ANDROID);
    }

    ctx.AddCommand<TAlarmShowDirective>(ACTION_SHOW_ALARMS, NSc::TValue{});
}

// If sound settings are supported, adds an attention and returns 'true'
bool AddAttentionIfSoundIsSupported(TContext& ctx) {
    if (ctx.ClientFeatures().SupportsSoundAlarms()) {
        if (ctx.ClientFeatures().SupportsChangeAlarmSound() ||
            ctx.HasExpFlag(TStringBuf("change_alarm_sound_debug_feature_flag"))
        ) {
            ctx.AddAttention(ATTENTION_SOUND_IS_SUPPORTED);
            return true;
        } else {
            ctx.AddAttention(ATTENTION_UPDATE_TO_SUPPORT_SOUND);
            return false;
        }
    }
    return false;
}

// If there are no alarms, adds an attention and returns 'true'
bool AddAttentionIfNoAlarms(TContext& ctx) {
    if (!GetAlarmsState(ctx)) {
        ctx.AddAttention(ATTENTION_NO_ALARMS);
        return true;
    }
    bool added = false;
    WithAlarms(ctx, [&](const TAlarmsContext& ac) {
        const auto activeAlarms = GetActiveAlarms(ac.Alarms, ac.NowInstant());
        if (activeAlarms.empty()) {
            ctx.AddAttention(ATTENTION_NO_ALARMS);
            added = true;
        }
    });
    return added;
}

// If default sound is set, adds an attention and returns 'true'
bool AddAttentionIfDefaultSoundIsSet(TContext& ctx) {
    const auto& state = ctx.Meta().DeviceState();
    if (state.HasAlarmsState() && state.AlarmsState().HasSoundAlarmSetting()) {
        return false;
    }
    ctx.AddAttention(ATTENTION_DEFAULT_SOUND_IS_SET);
    return true;
}

// Checks if music can be played and adds specific attentions
bool CanPlayMusic(TContext& ctx) {
    if (!ctx.IsAuthorizedUser()) {
        ctx.AddAttention(ATTENTION_UNAUTHORIZED);
        return false;
    }
    TPersonalDataHelper::TUserInfo userInfo;
    if (!TPersonalDataHelper(ctx).GetUserInfo(userInfo)) {
        ctx.AddAttention(ATTENTION_NO_USER_INFO);
        return false;
    }
    if (!userInfo.GetHasYandexPlus()) {
        ctx.AddAttention(ATTENTION_PAYMENT_REQUIRED);
        if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_SEND_PLUS_BUY_LINK)) {
            ctx.AddAttention(ATTENTION_PLUS_PUSH);
            ctx.SendPushRequest("music", "link_for_buy_plus", userInfo.GetUid(), {});
        }
        return false;
    }
    return true;
}

void AddSetSoundCommand(TContext& ctx, const NSc::TValue& info, const NSc::TValue& data, const NSc::TValue& slotData, const bool forRadio) {
    NSc::TValue payload;
    payload["sound_alarm_setting"]["type"] = forRadio ? TStringBuf("radio") : TStringBuf("music");
    payload["sound_alarm_setting"]["info"] = info;
    auto& serverAction = payload["server_action"].SetDict();
    serverAction["type"] = TStringBuf("server_action");

    if (ctx.HasExpFlag(EXPERIMANTAL_FLAG_ALARM_SEMANTIC_FRAME) && ctx.ClientFeatures().SupportsSemanticFrameAlarms() &&
        !forRadio)
    {
        serverAction["name"] = TStringBuf("@@mm_semantic_frame");
        serverAction["payload"]["typed_semantic_frame"]["music_play_semantic_frame"] = NSc::TValue::FromJsonValue(NAlice::JsonFromProto(
            NAlice::NMusic::ConstructMusicPlaySemanticFrame(
                (data.Has("filters") && !data["filters"].DictEmpty()) ? data.ToJsonValue() : data["object"].ToJsonValue(),
                !IsSlotEmpty(ctx.GetSlot(SLOT_REPEAT))
            )
        ));

        if (!IsSlotEmpty(ctx.GetSlot(SLOT_REPEAT))) {
            payload["sound_alarm_setting"]["repeat"].SetBool(true);
        }

        serverAction["payload"]["analytics"]["origin"] = "Scenario";
        serverAction["payload"]["analytics"]["purpose"] = "play_music_alarm";
        serverAction["payload"]["analytics"]["product_scenario"] = NAlice::NProductScenarios::ALARM;
    } else {
        serverAction["name"] = TStringBuf("bass_action");
        serverAction["payload"]["name"] = forRadio ? NRadio::QUASAR_RADIO_PLAY_OBJECT_ACTION_NAME : NMusic::QUASAR_MUSIC_PLAY_OBJECT_ACTION_NAME;
        serverAction["payload"]["data"] = data;
        serverAction["payload"]["@parent_product_scenario_name"] = NAlice::NProductScenarios::ALARM;
        if (!forRadio && !IsSlotEmpty(ctx.GetSlot(SLOT_REPEAT))) {
            payload["sound_alarm_setting"]["repeat"].SetBool(true);
            serverAction["payload"]["data"]["repeat"].SetBool(true);
        }
    }

    ctx.AddCommand<TAlarmSetSoundDirective>(ACTION_ALARM_SET_SOUND, std::move(payload), true /* beforeTts */);
    const auto resultSlotName = forRadio ? SLOT_RADIO_RESULT : SLOT_MUSIC_RESULT;
    ctx.CreateSlot(resultSlotName, resultSlotName, /* optional */ true, std::move(slotData));
    ctx.AddStopListeningBlock();
}

void AddSetSoundCommandWithObject(TContext& ctx, const NSc::TValue& info, const NSc::TValue& object, const NSc::TValue& slotData, const bool forRadio) {
    NSc::TValue data;
    data["object"] = object;
    AddSetSoundCommand(ctx, info, std::move(data), slotData, forRadio);
}

void AlarmSetSound(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (!AddAttentionIfSoundIsSupported(ctx)) {
        return;
    }

    // No alarms => attention
    AddAttentionIfNoAlarms(ctx);

    // Target can be: '', 'radio', 'track', 'album', 'artist', 'playlist'
    auto* slotTarget = ctx.GetSlot(SLOT_TARGET);
    const auto getTarget = [&]() { return IsSlotEmpty(slotTarget) ? "" : slotTarget->Value.GetString(); };
    const auto setTarget = [&](TStringBuf target) {
        if (!IsSlotEmpty(slotTarget)) {
            slotTarget->Value.SetString(target);
        } else {
            // We don't care about type here
            ctx.CreateSlot(SLOT_TARGET, SLOT_TARGET, /* optional */ true, NSc::TValue(target));
            slotTarget = ctx.GetSlot(SLOT_TARGET);
        }
    };

    // It will just explain how to set sound
    if (ctx.FormName() == FORM_NAME_HOW_TO_SET_SOUND) {
        ctx.AddAttention(ATTENTION_HOW_TO_SET_SOUND);
        return;
    }

    // Search slots
    auto* radioSearchSlot = ctx.GetSlot(SLOT_RADIO_SEARCH);
    auto* radioFreqSlot = ctx.GetSlot(SLOT_RADIO_FREQ);
    auto* musicSearchSlot = ctx.GetSlot(SLOT_MUSIC_SEARCH);
    auto* playlistSlot = ctx.GetSlot(SLOT_PLAYLIST);

    // Radio search
    if (!IsSlotEmpty(radioSearchSlot) || !IsSlotEmpty(radioFreqSlot)) {
        // Classifier may find both radio search and music target
        // Search always has priority
        setTarget(TARGET_TYPE_RADIO);

        const bool isFmRadio = !IsSlotEmpty(radioSearchSlot) && radioSearchSlot->Type == NRadio::TYPE_KNOWN_RADIO;
        const bool isFmRadioFreq = !IsSlotEmpty(radioFreqSlot) && radioFreqSlot->Type == NRadio::TYPE_KNOWN_RADIO_FREQ;

        if (isFmRadio || isFmRadioFreq) {
            TMaybe<TString> radioName = TRadioFormHandler::GetRadioName(ctx, isFmRadio, radioSearchSlot, radioFreqSlot, NAutomotive::TFMRadioDatabase());
            // Get radio id from the hardcoded map
            if (const auto radioId = TRadioFormHandler::GetRadioId(radioName)) {
                TMaybe<NSc::TValue> streamData;
                // Try to get stream data
                if (auto searchError = TRadioFormHandler::SearchRadioStream(ctx, radioId.GetRef(), NRadio::ESelectionMethod::Current, streamData)) {
                    NSc::TValue errData;
                    errData["code"].SetString(searchError->Msg);
                    ctx.AddAttention(ATTENTION_UNKNOWN_SOUND_ERROR, std::move(errData));
                    return;
                }
                if (streamData && streamData->Get("available").GetBool()) {
                    // We allow setting radio on alarm even if it isn't currently active
                    // Though we do add a warning in nlg
                    AddSetSoundCommandWithObject(ctx, *streamData, *streamData, *streamData, /* forRadio */ true);
                    return;
                }
                ctx.AddAttention(ATTENTION_RADIO_SEARCH_FAILURE);
                return;
            }
            ctx.AddCountedAttention(NRadio::ATTENTION_FM_STATION_IS_UNRECOGNIZED);
            ctx.AddAttention(ATTENTION_RADIO_SEARCH_FAILURE);
            return;
        }

        ctx.AddCountedAttention(NRadio::ATTENTION_FM_STATION_IS_UNRECOGNIZED);

        if (!IsSlotEmpty(radioSearchSlot)) {
            // Fallback to music search for unknown FM radio station
            if (IsSlotEmpty(musicSearchSlot)) {
                const auto searchText = radioSearchSlot->Value.GetString();
                musicSearchSlot = ctx.CreateSlot(SLOT_MUSIC_SEARCH, "string", /* optional */ true, searchText, searchText);
            }
            // No return here cause we use music search slot below
        } else if (!IsSlotEmpty(radioFreqSlot)) {
            ctx.AddAttention(ATTENTION_RADIO_SEARCH_FAILURE);
            return;
        } else {
            Y_FAIL();
        }
    }

    // Music search
    if (!IsSlotEmpty(musicSearchSlot) || !IsSlotEmpty(playlistSlot)) {
        // Classifier may find both music search and radio target
        // Search always has priority
        if (getTarget() == TARGET_TYPE_RADIO) {
            setTarget(TARGET_TYPE_TRACK);
        }
        if (!CanPlayMusic(ctx)) {
            return;
        }

        // Slot used in music intent
        // Note: we don't need to create 'playlist' as it already exists
        ctx.CreateSlot("search_text", "search_text", /* optional */ true, musicSearchSlot->Value, musicSearchSlot->SourceText);
        // We don't want filter slots to interfere when music search slot is present
        for (TStringBuf slotName : MUSIC_FILTER_SLOTS) {
            ctx.DeleteSlot(slotName);
        }
        NSc::TValue slotData;
        NMusic::FillSlotData(ctx, slotData);

        // Initialize provider
        NMusic::TQuasarProvider provider(ctx);
        if (!provider.InitRequestParams(slotData)) {
            ctx.AddErrorBlockWithCode(
                TError(TError::EType::MUSICERROR, TStringBuf("cannot_init_provider")),
                TStringBuf("params_problem")
            );
            return;
        }

        // Get music answer
        NSc::TValue outputValue;
        outputValue["for_alarm"].SetBool(true); // It's used in GetAnswer to avoid some music logic
        if (TResultValue error = GetAnswer(provider, ctx, &outputValue, slotData,
                                           /* actionData= */ {}, /* alarm= */ true)) {
            if (error->Type == TError::EType::MUSICERROR || error->Type == TError::EType::UNAUTHORIZED) {
                ctx.AddErrorBlockWithCode(*error, error->Msg);
            } else {
                NSc::TValue errData;
                errData["code"].SetString(error->Msg);
                ctx.AddAttention(ATTENTION_UNKNOWN_SOUND_ERROR, std::move(errData));
            }
            return;
        }
        if (outputValue.IsNull()) {
            ctx.AddErrorBlockWithCode(
                TError(TError::EType::MUSICERROR, TStringBuf("empty_service_answer")),
                NMusic::ERROR_MUSIC_NOT_FOUND
            );
            return;
        }

        AddSetSoundCommandWithObject(ctx, outputValue, outputValue, outputValue);
        return;
    }

    // Among music targets from context we only support 'track' for now
    // So if another one is asked by user, we change it and add an attention
    if (getTarget() == TARGET_TYPE_ALBUM ||
        getTarget() == TARGET_TYPE_ARTIST ||
        getTarget() == TARGET_TYPE_PLAYLIST
    ) {
        setTarget(TARGET_TYPE_TRACK);
        ctx.AddAttention(ATTENTION_SET_SONG_INSTEAD);
    }

    // Simply indicates that user wants current song/radio/album/etc
    auto* thisSlot = ctx.GetSlot(SLOT_THIS);

    // If 'this' is present, we ignore filters
    if (IsSlotEmpty(thisSlot)) {
        // Parse filter slots
        NSc::TValue filters;
        for (TStringBuf slotName : MUSIC_FILTER_SLOTS) {
            const auto* slot = ctx.GetSlot(slotName);
            if (IsSlotEmpty(slot) || !slot->Value.IsString() || slot->Value.GetString().empty()) {
                continue;
            }
            // We need 'filters' subnode to check for it later in nlg and in server action handler
            filters["filters"][slotName] = slot->Value;
        }
        if (filters.Has("filters") && !filters["filters"].DictEmpty()) {
            // If genre, mood, activity, epoch or personality is provided then target is musical
            setTarget(TARGET_TYPE_TRACK);
            if (!CanPlayMusic(ctx)) {
                return;
            }

            AddSetSoundCommand(ctx, filters, filters, filters);
            return;
        }
    }

    const auto& state = ctx.Meta().DeviceState();

    TMaybe<NVideo::EScreenId> screen = NVideo::GetCurrentScreen(ctx);
    const bool showingOnTv = state.IsTvPluggedIn() && screen;
    using TVideoState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;

    // Check the context: what is playing and what screen is shown (if any)
    if (auto radioInfo = *state.Radio().CurrentlyPlaying().GetRawValue();
        !radioInfo.IsNull() && (!NPlayer::IsRadioPaused(ctx) || (showingOnTv && screen == NVideo::EScreenId::RadioPlayer))
    ) {
        // Radio is playing (or radio screen is shown)
        // Target is deduced from that
        if (!getTarget()) {
            setTarget(TARGET_TYPE_RADIO);
        }

        // Add a command and update slots if context and target are compatible
        if (getTarget() == TARGET_TYPE_RADIO) {
            NSc::TValue radioResult;
            // Same format as in radio search
            radioResult["title"] = radioInfo["radioTitle"];
            radioResult["radioId"] = radioInfo["radioId"];
            radioResult["active"].SetBool(true);
            radioResult["available"].SetBool(true);
            AddSetSoundCommandWithObject(ctx, radioResult, radioResult, radioResult, /* forRadio */ true);
        } else {
            // Can't play a song/artist/album from a radio yet
            ctx.AddAttention(ATTENTION_MUSIC_FROM_RADIO);
        }
    } else if (!NPlayer::IsMusicPaused(ctx) && DoesMusicPlayerHaveTrackInfo(state)) {
        SetAlarmMusicPlayerTrack(ctx, state.Music(), getTarget());

    } else if (IsAudioPlayerPlaying(state) && DoesAudioPlayerHaveTrackInfo(state)) {
        SetAlarmAudioPlayerTrack(ctx, state.AudioPlayer(), getTarget());

    } else if ((showingOnTv && screen == NVideo::EScreenId::MusicPlayer) &&
               DoesMusicPlayerHaveTrackInfo(state) && DoesAudioPlayerHaveTrackInfo(state)) {
        // Both players are on pause, both have info about music track, we have to pick one of them
        const auto& musicPlayer = state.Music();
        const auto& audioPlayer = state.AudioPlayer();
        if (musicPlayer.LastPlayTimestamp() >= audioPlayer.LastPlayTimestamp()) {
            SetAlarmMusicPlayerTrack(ctx, musicPlayer, getTarget());
        } else {
            SetAlarmAudioPlayerTrack(ctx, audioPlayer, getTarget());
        }
    } else if ((showingOnTv && screen == NVideo::EScreenId::MusicPlayer) && DoesMusicPlayerHaveTrackInfo(state)) {
        // Both players are on pause, but only MusicPlayer has info about the track
        const auto& musicPlayer = state.Music();
        SetAlarmMusicPlayerTrack(ctx, musicPlayer, getTarget());

    } else if ((showingOnTv && screen == NVideo::EScreenId::MusicPlayer) && DoesAudioPlayerHaveTrackInfo(state)) {
        // Both players are on pause, but only AudioPlayer has info about the track
        const auto& audioPlayer = state.AudioPlayer();
        SetAlarmAudioPlayerTrack(ctx, audioPlayer, getTarget());

    } else if (TVideoState videoInfo(state.Video().CurrentlyPlaying().GetRawValue());
        showingOnTv && screen == NVideo::EScreenId::VideoPlayer && !videoInfo.Paused()
    ) {
        // No targets are compatible with a playing video context yet
        if (getTarget() == TARGET_TYPE_RADIO) {
            // Video is not a radio
            ctx.AddAttention(ATTENTION_NOT_A_RADIO);
        } else {
            // Can't play a song/artist/album from a video yet
            ctx.AddAttention(ATTENTION_MUSIC_FROM_VIDEO);
        }
    } else if (IsSlotEmpty(thisSlot)) {
        // If 'this' isn't present, there is no search, no filters and no context,
        // we ask the user what is it exactly that she wants to set on alarm
        // For that reason we set 'this' as not optional as part of a group of slots
        if (!thisSlot) {
            thisSlot = ctx.CreateSlot(SLOT_THIS, TYPE_CURRENT);
        }
        thisSlot->Optional = false;
    }
}

void AlarmResetSound(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (!AddAttentionIfSoundIsSupported(ctx)) {
        return;
    }

    // Default sound => attention
    if (AddAttentionIfDefaultSoundIsSet(ctx)) {
        return;
    }

    // A command for the speaker
    ctx.AddCommand<TAlarmResetSoundDirective>(ACTION_ALARM_RESET_SOUND, {} /* data */, true /* beforeTts */);
}

void AlarmWhatSoundIsSet(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (!AddAttentionIfSoundIsSupported(ctx)) {
        return;
    }

    // No alarms => attention
    AddAttentionIfNoAlarms(ctx);

    // Default sound => attention
    if (AddAttentionIfDefaultSoundIsSet(ctx)) {
        return;
    }

    auto soundAlarmSetting = ctx.Meta().DeviceState().AlarmsState().SoundAlarmSetting().GetRawValue()->Clone();
    const auto slot = soundAlarmSetting["type"] == "radio" ? SLOT_RADIO_RESULT : SLOT_MUSIC_RESULT;
    if (slot != SLOT_RADIO_RESULT && soundAlarmSetting.Has("repeat") && soundAlarmSetting["repeat"].GetBool()) {
        ctx.AddAttention(ATTENTION_REPEAT_SOUND);
    }
    ctx.CreateSlot(slot, slot, /* optional */ true, std::move(soundAlarmSetting["info"]));
}

void AlarmSetWithSound(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    // First set time
    AlarmSet(ctx);
    // Stop here if time wasn't set
    const auto* const timeSlot = ctx.GetSlot(SLOT_TIME);
    if (ctx.HasAnyErrorBlock() ||
        timeSlot && timeSlot->Value.IsNull() && !timeSlot->Optional ||
        ctx.HasAttention(ATTENTION_ALARM_NEED_CONFIRMATION)
    ) {
        return;
    }

    // This is needed for nlg
    ctx.AddAttention(ATTENTION_ALARM_SET_SUCCESS);

    // Then set sound
    AlarmSetSound(ctx);
    // Check for expected errors and turn them into attentions, cause we
    // don't want to completely fail when we only failed in setting sound
    // Unexpected errors will still cause operation failure and it's fine
    TVector<NSc::TValue> deletedErrors = ctx.DeleteErrorBlocks({
        TError::EType::MUSICERROR, TError::EType::UNAUTHORIZED });
    for (const auto& errorBlock : deletedErrors) {
        ctx.AddAttention(
            TString{TStringBuf("deleted_error__")} + errorBlock["error"]["type"].ForceString(),
            errorBlock["data"]
        );
    }
}

void AlarmSoundSetLevel(TContext& ctx) {
    SetAlarmProductScenario(ctx);
    if (ctx.ClientFeatures().SupportsChangeAlarmSoundLevel()) {
        NSound::SetLevel(ctx, NSound::MakeLevelDirectiveAdder<TAlarmSetMaxLevelDirective>(
            TStringBuf("alarm_set_max_level") /* type */,
            true /* beforeTts */
        ));
    } else {
        ctx.AddErrorBlock(TError::EType::NOTSUPPORTED);
    }
}

void AlarmWhatSoundLevelIsSet(TContext& ctx) {
    constexpr i64 DEFAULT_MAX_ALARM_LEVEL = 7;

    SetAlarmProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    i64 maxSoundLevel = ctx.Meta().DeviceState().AlarmsState().MaxSoundLevel();
    if (maxSoundLevel <= 0) {
        maxSoundLevel = DEFAULT_MAX_ALARM_LEVEL;
    }
    ctx.CreateSlot(SLOT_SOUND_LEVEL, TYPE_NUM, /* optional */ true, maxSoundLevel);
}

} // namespace NReminders
} // namespace NBASS
