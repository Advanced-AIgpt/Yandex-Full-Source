#include "alarm_cases.h"

#include <alice/hollywood/library/music/create_search_request.h>
#include <alice/hollywood/library/music/fm_radio.h>
#include <alice/hollywood/library/music/music_catalog.h>
#include <alice/hollywood/library/sound/sound_level_calculation.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>

#include <alice/library/apphost_request/request_builder.h>
#include <alice/library/music/catalog.h>
#include <alice/library/url_builder/url_builder.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

#include <alice/protos/data/language/language.pb.h>

#include <alice/protos/data/scenario/alice_show/selectors.pb.h>

namespace {

constexpr TStringBuf MORNING_SHOW_TYPE = "morning_show";
constexpr TStringBuf ALICE_SHOW_TYPE = "alice_show";

constexpr size_t MAX_NUM_ALARMS = 25;

void FillParsedUtteranceForMorningShow(NAlice::NScenarios::TParsedUtterance* utterance) {
    auto* morningShow = utterance->MutableTypedSemanticFrame()
                                 ->MutableAliceShowActivateSemanticFrame();

    morningShow->MutableDayPart()->SetDayPartValue(NAlice::NData::NAliceShow::TDayPart::Morning);
}

bool ClientSupportsAlarms(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    return ctx.GetInterfaces().GetCanSetAlarm();
}

bool AddAttentionIfSoundIsSupported(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    if (ClientSupportsAlarms(ctx)) {
        if (ctx.GetInterfaces().GetCanChangeAlarmSound() ||
            ctx.RunRequest().HasExpFlag(NAlice::NHollywood::NReminders::NExperiments::CHANGE_ALARM_SOUND_DEBUG_FEATURE)) {
            ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_SOUND_IS_SUPPORTED);
            return true;
        } else {
            ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_UPDATE_TO_SUPPORT_SOUND);
            return false;
        }
    }

    return false;
}

bool AddAttentionIfDefaultSoundIsSet(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    const auto& deviceState = ctx.GetDeviceState();
    if (deviceState.HasAlarmState() && deviceState.GetAlarmState().HasSoundAlarmSetting()) {
        return false;
    }
    ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_DEFAULT_SOUND_IS_SET);
    return true;
}

bool CanPlayMusic(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    auto userInfo = GetUserInfoProto(ctx.RunRequest());
    if (!userInfo) {
        ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_UNAUTHORIZED);
        return false;
    }

    if (!userInfo->GetHasYandexPlus()) {
        ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_PAYMENT_REQUIRED);
        return false;
    }

    return true;
}

void SetAlarmProductScenario(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    ctx.Renderer().SetProductScenarioName(NAlice::NHollywood::NReminders::NScenarioNames::ALARM.Data());
}

TMaybe<TString> GetAlarmsState(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    const auto& deviceState = ctx.GetDeviceState();

    if (deviceState.HasAlarmState()) {
        const auto& state = deviceState.GetAlarmState();
        if (!state.HasICalendar()) {
            return Nothing();
        }
        return ToString(state.GetICalendar());
    }

    if (!deviceState.HasAlarmsState()) {
        return Nothing();
    }

    return ToString(deviceState.GetAlarmsState());
}

TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm> GetCalendarItems(
    const NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const NDatetime::TTimeZone& tz,
    TStringBuf data,
    TInstant now
) {
    auto alarms = NAlice::NScenarios::NAlarm::TWeekdaysAlarm::FromICalendar(tz, data);
    if (!ctx.RunRequest().HasExpFlag(NAlice::NHollywood::NReminders::NExperiments::EXPERIMENTAL_FLAG_ALARMS_KEEP_OBSOLETE)) {
        EraseIf(alarms, [now](const NAlice::NScenarios::NAlarm::TWeekdaysAlarm& alarm) { return alarm.IsOutdated(now); });
    }
    return alarms;
}

struct TActiveAlarm {
    TActiveAlarm(const NAlice::NScenarios::NAlarm::TWeekdaysAlarm& alarm, size_t index)
        : Alarm(alarm), Index(index) {
    }

    bool operator<(const TActiveAlarm& rhs) const {
        return Alarm.Begin < rhs.Alarm.Begin;
    }


    NJson::TJsonValue ToJson(const NDatetime::TCivilSecond& now, const NDatetime::TTimeZone& tz) const {
        const auto begin = NDatetime::Convert(Alarm.Begin, tz);
        NJson::TJsonValue json;
        json["id"] = Index;
        json["time"] = NAlice::NScenarios::NAlarm::TimeToValue(begin).ToJsonValue();

        if (const TMaybe<NAlice::NScenarios::NAlarm::TWeekdays> weekdays = Alarm.GetLocalWeekdays(tz)) {
            json["date"] = weekdays->ToValue().ToJsonValue();
        } else {
            json["date"] = NAlice::NScenarios::NAlarm::DateToValue(now, begin).ToJsonValue();
        }

        return json;
    }

    NAlice::NScenarios::NAlarm::TWeekdaysAlarm Alarm;
    size_t Index = 0;
};

NJson::TJsonValue ToJson(
    const TVector<TActiveAlarm>& alarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    NJson::TJsonValue json;
    for (const auto& alarm : alarms) {
        json.AppendValue(alarm.ToJson(now, tz));
    }

    return json;
}

TVector<TActiveAlarm> GetActiveAlarms(
    const TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const TInstant& now
) {
    TVector<TActiveAlarm> result;

    for (size_t i = 0; i < alarms.size(); ++i) {
        const auto alarm = alarms[i].GetRest(now);
        if (alarm) {
            result.emplace_back(*alarm, i);
        }
    }

    return result;
}

bool AddAttentionIfNoAlarms(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    const auto alarmsState = GetAlarmsState(ctx);
    if (!alarmsState) {
        ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_NO_ALARMS);
        return true;
    }

    const TInstant now = GetCurrentTimestamp(ctx);
    const NDatetime::TTimeZone timeZone = NDatetime::GetTimeZone(ctx.GetTimezone());
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm> alarms = GetCalendarItems(ctx, timeZone, *alarmsState, now);
    if (GetActiveAlarms(alarms, now).empty()) {
        ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_NO_ALARMS);
        return true;
    }

    return false;
}

TVector<TActiveAlarm> FilterApproximateMatchingDayTimeAndDate(
    const TVector<TActiveAlarm>& activeAlarms,
    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime>& dayTime,
    const TMaybe<NAlice::NScenarios::NAlarm::TDate>& date,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    TVector<TActiveAlarm> matchingAlarms;

    for (const auto& activeAlarm : activeAlarms) {
        const auto& alarm = activeAlarm.Alarm;
        if (NAlice::NScenarios::NAlarm::MatchesApproximatelyDayTimeAndDate(alarm, dayTime, date, now, tz)) {
            matchingAlarms.push_back(activeAlarm);
        }
    }

    return matchingAlarms;
}

TVector<TActiveAlarm> FilterApproximateMatchingDayTimeAndWeekdays(
    const TVector<TActiveAlarm>& activeAlarms,
    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime>& dayTime,
    const NAlice::NScenarios::NAlarm::TWeekdays& weekdays,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    TVector<TActiveAlarm> matchingAlarms;

    for (const auto& activeAlarm : activeAlarms) {
        const auto& alarm = activeAlarm.Alarm;
        if (NAlice::NScenarios::NAlarm::MatchesApproximatelyDayTimeAndWeekdays(alarm, dayTime, weekdays, now, tz)) {
            matchingAlarms.push_back(activeAlarm);
        }
    }

    return matchingAlarms;
}

TVector<TActiveAlarm> FilterApproximateMatchingDayTime(
    const TVector<TActiveAlarm>& activeAlarms,
    const NAlice::NScenarios::NAlarm::TDayTime& dayTime,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz
) {
    TVector<TActiveAlarm> matchingAlarms;

    for (const auto& activeAlarm : activeAlarms) {
        const auto& alarm = activeAlarm.Alarm;
        if (NAlice::NScenarios::NAlarm::MatchesApproximatelyDayTime(alarm, dayTime, now, tz)) {
            matchingAlarms.push_back(activeAlarm);
        }
    }

    return matchingAlarms;
}

template <typename T>
bool FromSlotImpl(const NAlice::NHollywood::TPtrWrapper<NAlice::NHollywood::TSlot>& slot, TMaybe<T>& result) {
    if (NAlice::NHollywood::NReminders::IsSlotEmpty(slot)) {
        result.Clear();
        return true;
    }

    const TMaybe<T> r = T::FromValue(NSc::TValue::FromJson(slot->Value.AsString()));
    if (!r) {
        return false;
    }

    result = r;
    return true;
}

template <typename T>
bool FromSlot(const NAlice::NHollywood::TPtrWrapper<NAlice::NHollywood::TSlot>& slot, TMaybe<T>& result) {
    return FromSlotImpl(slot, result);
}

template <>
bool FromSlot(const NAlice::NHollywood::TPtrWrapper<NAlice::NHollywood::TSlot>& slot, TMaybe<NAlice::NScenarios::NAlarm::TDate>& result) {
    TMaybe<NAlice::NScenarios::NAlarm::TDate> date;
    if (!FromSlotImpl(slot, date)) {
        return false;
    }

    if (!date || date->HasExactDay()) {
        result = date;
        return true;
    }

    return false;
}

constexpr int SUGGESTED_HOURS[] = {7, 6, 8};

void AddAlarmSetSuggests(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    if (ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
        return;
    }

    for (const auto& hours : SUGGESTED_HOURS) {
        NJson::TJsonValue suggestData;
        suggestData["time"]["hours"] = hours;
        ctx.Renderer().AddTypeTextSuggest(
            "set_alarm",
            suggestData,
            TString(TStringBuilder{} << "suggest_set_alarm_on_" << hours)
        );
    }
}

void AddAlarmShowSuggest(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    if (ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
        return;
    }

    NJson::TJsonValue suggestData;
    suggestData["uri"] = NAlice::TClientActionUrl(NAlice::TClientActionUrl::EType::ShowAlarms).ToString();
    ctx.Renderer().AddOpenUriSuggest(
        "show_alarms",
        suggestData,
        "suggest_show_alarms",
        true /* addButton */
    );
}

void AskAboutAlarm(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const TActiveAlarm& activeAlarm
) {
    auto availableAlarmsSlot = NAlice::NHollywood::NReminders::FindOrAddSlot(
        frame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_ALARMS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_LIST
    );
    const_cast<NAlice::NHollywood::TSlot*>(availableAlarmsSlot.Get())->Value = NAlice::NHollywood::TSlot::TValue(
        NAlice::JsonToString(ToJson({activeAlarm}, now, tz))
    );

    auto alarmIdSlot = NAlice::NHollywood::NReminders::FindOrAddSlot(
        frame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_ALARM_ID,
        NAlice::NHollywood::NReminders::NSlots::TYPE_NUM
    );
    const_cast<NAlice::NHollywood::TSlot*>(alarmIdSlot.Get())->Value = NAlice::NHollywood::TSlot::TValue(
        NAlice::JsonToString(NJson::TJsonValue(1))
    );

    ctx.State().MutableSemanticFrame()->MergeFrom(frame.ToProto());
    ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_NO_ALARMS);

    NJson::TJsonValue cardData;
    cardData[NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_ALARMS] = ToJson({activeAlarm}, now, tz);

    ctx.Renderer().AddVoiceCard(
        NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    ctx.Renderer().SetShouldListen(true);

    return ConstructResponse(ctx);
}

void AskAboutAlarms(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const TVector<TActiveAlarm>& activeAlarms
) {
    if (activeAlarms.size() == 1) {
        return AskAboutAlarm(ctx, frame, now, tz, activeAlarms[0]);
    }

    auto availableAlarmsSlot = NAlice::NHollywood::NReminders::FindOrAddSlot(
        frame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_ALARMS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_LIST
    );
    const_cast<NAlice::NHollywood::TSlot*>(availableAlarmsSlot.Get())->Value = NAlice::NHollywood::TSlot::TValue(
        NAlice::JsonToString(ToJson(activeAlarms, now, tz))
    );
    ctx.State().MutableSemanticFrame()->MergeFrom(frame.ToProto());


    ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_NO_ALARMS);

    NJson::TJsonValue cardData;
    cardData[NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_ALARMS] = ToJson(activeAlarms, now, tz);

    ctx.Renderer().AddVoiceCard(
        NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );
    ctx.Renderer().SetShouldListen(true);

    return ConstructResponse(ctx);
}

void CancelAlarm(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    size_t index
) {
    if (index >= alarms.size()) {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );

        return ConstructResponse(ctx);
    }

    const auto& alarm = alarms[index];
    const auto begin = NDatetime::Convert(alarm.Begin, tz);

    NJson::TJsonValue cardData;
    cardData["time"] = NAlice::NScenarios::NAlarm::TimeToValue(begin).ToJsonValue();

    if (const TMaybe<NAlice::NScenarios::NAlarm::TWeekdays>& weekdays = alarm.GetLocalWeekdays(tz); weekdays) {
        cardData["date"] = weekdays->ToValue().ToJsonValue();
    } else {
        cardData["date"] = NAlice::NScenarios::NAlarm::DateToValue(now, begin).ToJsonValue();
    }

    alarms.erase(alarms.begin() + index);

    NAlice::NScenarios::TDirective directive;
    directive.MutableAlarmsUpdateDirective();
    auto* alarmsUpdateDirective = directive.MutableAlarmsUpdateDirective();
    alarmsUpdateDirective->SetState(NAlice::NScenarios::NAlarm::TWeekdaysAlarm::ToICalendar(alarms));
    alarmsUpdateDirective->SetListeningIsPossible(true);

    ctx.Renderer().AddDirective(std::move(directive));

    ctx.Renderer().AddVoiceCard(
        NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    return ConstructResponse(ctx);
}

void CancelAlarms(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    TVector<size_t> indices
) {
    SortUnique(indices);

    if (indices.size() == 1) {
        return CancelAlarm(
            ctx,
            alarms,
            now,
            tz,
            indices[0]
        );
    }

    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm> result;

    try {
        result = RemoveByIndices(alarms, indices);
    } catch (const yexception& e) {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );

        return ConstructResponse(ctx);
    }

    NAlice::NScenarios::TDirective directive;
    directive.MutableAlarmsUpdateDirective();
    auto* alarmsUpdateDirective = directive.MutableAlarmsUpdateDirective();
    alarmsUpdateDirective->SetState(NAlice::NScenarios::NAlarm::TWeekdaysAlarm::ToICalendar(result));
    alarmsUpdateDirective->SetListeningIsPossible(true);

    ctx.Renderer().AddDirective(std::move(directive));

    NJson::TJsonValue cardData;
    cardData["cancel_alarms"] = NJson::TJsonValue(true);
    ctx.Renderer().AddVoiceCard(
        NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    return ConstructResponse(ctx);
}

void CancelAllAlarms(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx
) {
    NAlice::NScenarios::TDirective directive;
    directive.MutableAlarmsUpdateDirective();
    auto* alarmsUpdateDirective = directive.MutableAlarmsUpdateDirective();
    alarmsUpdateDirective->SetState(NAlice::NScenarios::NAlarm::TWeekdaysAlarm::ToICalendar({}));
    alarmsUpdateDirective->SetListeningIsPossible(true);

    ctx.Renderer().AddDirective(std::move(directive));


    NJson::TJsonValue cardData;
    cardData["cancel_all_alarms"] = NJson::TJsonValue(true);
    ctx.Renderer().AddVoiceCard(
        NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    return ConstructResponse(ctx);
}

void CancelSingle1Set(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const TActiveAlarm& activeAlarm,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime>& dayTime,
    const TMaybe<NAlice::NScenarios::NAlarm::TDate>& date
) {
    const auto& alarm = activeAlarm.Alarm;
    if (!dayTime && !date) {
        return CancelAlarm(
            ctx,
            alarms,
            now,
            tz,
            activeAlarm.Index
        );
    }

    if (MatchesExactlyDayTimeAndDate(alarm, dayTime, date, now, tz)) {
        return CancelAlarm(
            ctx,
            alarms,
            now,
            tz,
            activeAlarm.Index
        );
    }

    return AskAboutAlarm(ctx, frame, now, tz, activeAlarm);
}

void CancelSingleManySet(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const TVector<TActiveAlarm>& activeAlarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime>& dayTime,
    const TMaybe<NAlice::NScenarios::NAlarm::TDate>& date
) {
    Y_ASSERT(activeAlarms.size() > 1);

    if (!dayTime && !date) {
        return AskAboutAlarms(ctx, frame, now, tz, activeAlarms);
    }

    TVector<TActiveAlarm> matchingAlarms;
    if (dayTime) {
        if (date) {
            matchingAlarms = FilterApproximateMatchingDayTimeAndDate(
                activeAlarms,
                dayTime,
                date,
                now,
                tz
            );
        } else {
            matchingAlarms = FilterApproximateMatchingDayTime(
                activeAlarms,
                *dayTime,
                now,
                tz
            );
        }
    } else {
        Y_ASSERT(date);

        matchingAlarms = FilterApproximateMatchingDayTimeAndDate(
            activeAlarms,
            Nothing(), /* dayTime */
            date,
            now,
            tz
        );
    }

    if (matchingAlarms.empty()) {
        return AskAboutAlarms(ctx, frame, now, tz, activeAlarms);
    }

    if (matchingAlarms.size() == 1) {
        if (MatchesExactlyDayTimeAndDate(matchingAlarms[0].Alarm, dayTime, date, now, tz)) {
            return CancelAlarm(
                ctx,
                alarms,
                now,
                tz,
                matchingAlarms[0].Index
            );
        }

        if (matchingAlarms[0].Alarm.IsRegular()) {
            return AskAboutAlarm(ctx, frame, now, tz, matchingAlarms[0]);
        }
    }

    return AskAboutAlarms(ctx, frame, now, tz, matchingAlarms);
}

void CancelRegular1Set(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const TActiveAlarm& activeAlarm,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime>& dayTime,
    const NAlice::NScenarios::NAlarm::TWeekdays& weekdays
) {
    if (MatchesExactlyDayTimeAndWeekdays(activeAlarm.Alarm, dayTime, weekdays, now, tz)) {
        return CancelAlarm(
            ctx,
            alarms,
            now,
            tz,
            activeAlarm.Index
        );
    }

    return AskAboutAlarm(ctx, frame, now, tz, activeAlarm);
}

void CancelRegularManySet(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const TVector<TActiveAlarm>& activeAlarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime>& dayTime,
    const NAlice::NScenarios::NAlarm::TWeekdays& weekdays
) {
    Y_ASSERT(activeAlarms.size() > 1);
    const auto matchingAlarms = FilterApproximateMatchingDayTimeAndWeekdays(
        activeAlarms,
        dayTime,
        weekdays,
        now,
        tz
    );

    if (matchingAlarms.empty()) {
        return AskAboutAlarms(ctx, frame, now, tz, activeAlarms);
    }

    if (matchingAlarms.size() == 1) {
        if (MatchesExactlyDayTimeAndWeekdays(matchingAlarms[0].Alarm, dayTime, weekdays, now, tz)) {
            return CancelAlarm(
                ctx,
                alarms,
                now,
                tz,
                matchingAlarms[0].Index
            );
        }
    }

    return AskAboutAlarms(ctx, frame, now, tz, matchingAlarms);
}

void CancelAllOnDayTimeAndDate(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const TVector<TActiveAlarm>& activeAlarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime>& dayTime,
    const TMaybe<NAlice::NScenarios::NAlarm::TDate>& date
) {
    const auto matchingAlarms = FilterApproximateMatchingDayTimeAndDate(
        activeAlarms,
        dayTime,
        date,
        now,
        tz
    );

    if (matchingAlarms.empty()) {
        return AskAboutAlarms(ctx, frame, now, tz, activeAlarms);
    }

    bool allSingle = true;
    for (const auto& matchingAlarm : matchingAlarms) {
        if (!matchingAlarm.Alarm.TriggersOnlyOnDate(date, now, tz)) {
            allSingle = false;
            break;
        }
    }

    if (!allSingle) {
        return AskAboutAlarms(ctx, frame, now, tz, matchingAlarms);
    }

    TVector<size_t> indices;
    for (const auto& matchingAlarm : matchingAlarms) {
        indices.push_back(matchingAlarm.Index);
    }

    return CancelAlarms(
        ctx,
        alarms,
        now,
        tz,
        indices
    );
}

void CancelById(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const NJson::TJsonValue& alarmIdValue
) {
    auto availableAlarmsSlot = NAlice::NHollywood::NReminders::FindSlot(
        frame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_ALARMS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_LIST
    );

    if (NAlice::NHollywood::NReminders::IsSlotEmpty(availableAlarmsSlot)) {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );

        return ConstructResponse(ctx);
    }

    const auto availableAlarms = NAlice::JsonFromString(availableAlarmsSlot->Value.AsString()).GetArray();
    if (availableAlarms.empty()) {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );

        return ConstructResponse(ctx);
    }

    i64 id = alarmIdValue.IsInteger() ? alarmIdValue.GetInteger() : -1;
    if (id < 0 || static_cast<ui64>(id) > availableAlarms.size()) {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::INVALID_ID
        );

        return ConstructResponse(ctx);
    }

    // Need to decrement here because alarmId is usually "first",
    // "second", etc., i.e. 1-based.
    if (id != 0) {
        --id;
    }

    Y_ASSERT(id >= 0 && static_cast<ui64>(id) < availableAlarms.size());
    TMaybe<i64> alarmId;
    // comparing by availableAlarm["id"] invalid in session with more than one cancel by id
    for (ui32 curId = 0; curId < alarms.size(); ++curId) {
        // alarm.Begin -- setting time for alarm from device state
        // availableAlarm["time"] -- setting time for alarm from state.semantic_frame
        if (NAlice::NScenarios::NAlarm::TimeToValue(NDatetime::Convert(alarms[curId].Begin, tz)).ToJsonValue() == availableAlarms[id]["time"]) {
            alarmId = curId;
            break;
        }
    }

    if (!alarmId.Defined()) {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::INVALID_ID
        );

        return ConstructResponse(ctx);
    }

    return CancelAlarm(
        ctx,
        alarms,
        now,
        tz,
        *alarmId
    );
}

void CancelBySelection(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    TVector<NAlice::NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const TVector<TActiveAlarm>& activeAlarms,
    const NDatetime::TCivilSecond& now,
    const NDatetime::TTimeZone& tz,
    const NJson::TJsonValue alarmSelectionValue
) {
    if (alarmSelectionValue.GetString() != "all") {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );

        return ConstructResponse(ctx);
    }

    auto availableAlarmsSlot = NAlice::NHollywood::NReminders::FindSlot(
        frame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_ALARMS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_LIST
    );

    if (NAlice::NHollywood::NReminders::IsSlotEmpty(availableAlarmsSlot)) {
        auto timeSlot = NAlice::NHollywood::NReminders::FindSlot(
            frame,
            NAlice::NHollywood::NReminders::NSlots::SLOT_TIME,
            NAlice::NHollywood::NReminders::NSlots::TYPE_TIME
        );

        auto dateSlot = NAlice::NHollywood::NReminders::FindSlot(
            frame,
            NAlice::NHollywood::NReminders::NSlots::SLOT_DATE,
            NAlice::NHollywood::NReminders::NSlots::TYPE_DATE
        );

        auto weekdaysSlot = NAlice::NHollywood::NReminders::FindSlot(
            frame,
            NAlice::NHollywood::NReminders::NSlots::SLOT_DATE,
            NAlice::NHollywood::NReminders::NSlots::TYPE_WEEKDAYS
        );

        if (NAlice::NHollywood::NReminders::IsSlotEmpty(timeSlot) &&
            NAlice::NHollywood::NReminders::IsSlotEmpty(dateSlot) &&
            NAlice::NHollywood::NReminders::IsSlotEmpty(weekdaysSlot)) {
            return CancelAllAlarms(ctx);
        }

        if (NAlice::NHollywood::NReminders::IsSlotEmpty(dateSlot) ||
            !NAlice::NHollywood::NReminders::IsSlotEmpty(weekdaysSlot)) {
            ctx.Renderer().AddVoiceCard(
                NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
            );

            return ConstructResponse(ctx);
        }

        TMaybe<NAlice::NScenarios::NAlarm::TDayTime> time;
        TMaybe<NAlice::NScenarios::NAlarm::TDate> date;

        if (!FromSlot(timeSlot, time) || !FromSlot(dateSlot, date)) {
            ctx.Renderer().AddVoiceCard(
                NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
            );

            return ConstructResponse(ctx);
        }

        Y_ASSERT(date);
        return CancelAllOnDayTimeAndDate(
            ctx,
            frame,
            alarms,
            activeAlarms,
            now,
            tz,
            time,
            *date
        );
    }

    const auto availableAlarms = NAlice::JsonFromString(availableAlarmsSlot->Value.AsString()).GetArray();
    if (availableAlarms.empty()) {
        ctx.Renderer().AddVoiceCard(
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );

        return ConstructResponse(ctx);
    }

    TVector<size_t> indices;
    for (auto availableAlarm : availableAlarms) {
        indices.emplace_back(availableAlarm["id"].GetUInteger());
    }

    return CancelAlarms(
        ctx,
        alarms,
        now,
        tz,
        indices
    );
}

TVector<TActiveAlarm> GetPossibleAlarms(const TInstant& now, const NDatetime::TTimeZone& tz, const NAlice::NScenarios::NAlarm::TDayTime& dayTime) {
    if (dayTime.Period == NAlice::NScenarios::NAlarm::TDayTime::EPeriod::Unspecified && !dayTime.IsRelative()) {
        if (!dayTime.Hours.Defined()) {
            static const auto setRelative = [](auto& comp) {
                if (comp) {
                    comp->Relative = true;
                }
            };

            // Create relative version of dayTime
            NAlice::NScenarios::NAlarm::TDayTime relDayTime = dayTime;
            setRelative(relDayTime.Hours);
            setRelative(relDayTime.Minutes);
            setRelative(relDayTime.Seconds);

            return {
                {NAlice::NScenarios::NAlarm::GetAlarmTime(now, tz, dayTime), 1},
                {NAlice::NScenarios::NAlarm::GetAlarmTime(now, tz, relDayTime), 2}
            };
        }
    }

    return {{NAlice::NScenarios::NAlarm::GetAlarmTime(now, tz, dayTime), 1}};
}

void AlarmSetConfirmation(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TMaybe<NAlice::NHollywood::TFrame>& frame,
    const NAlice::NHollywood::TPtrWrapper<NAlice::NHollywood::TSlot>& successSlot,
    const TStringBuf nlgTemplate
) {
    ctx.Renderer().AddFrameSlots(*frame);
    AddAlarmShowSuggest(ctx);

    const NJson::TJsonValue successJson = NAlice::JsonFromString(successSlot->Value.AsString());
    if (!successJson.IsBoolean()) {
        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );
    } else {
        const auto timeSlot = frame->FindSlot(NAlice::NHollywood::NReminders::NSlots::SLOT_TIME);
        if (!successJson.GetBoolean() || NAlice::NHollywood::NReminders::IsSlotEmpty(timeSlot)) {
            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::SETTING_FAILED
            );
        } else {
            NJson::TJsonValue cardData;
            cardData["time"] = NAlice::JsonFromString(timeSlot->Value.AsString());

            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
                cardData
            );
        }
    }
}

void AddSetSoundCommand(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const NJson::TJsonValue& object,
    const bool repeat,
    const bool forRadio
) {
    NAlice::NScenarios::TDirective directive;
    auto* alarmSetSoundDirective = directive.MutableAlarmSetSoundDirective();

    NJson::TJsonValue cardData;
    {
        auto* soundAlarmSetting = alarmSetSoundDirective->MutableSettings();
        soundAlarmSetting->SetType(forRadio ? "radio" : "music");
        soundAlarmSetting->SetRepeat(repeat);

        {
            auto* info = soundAlarmSetting->MutableInfo();
            NAlice::JsonToProto(
                object,
                *info,
                /* validateUtf8 = */ true,
                /* ignorreUnknownFields = */ true
            );

            cardData[(forRadio ? "radio_result" : "music_result")] = object;
        }
    }

    {
        auto* callback = alarmSetSoundDirective->MutableCallback();
        callback->SetName("@@mm_semantic_frame");

        NAlice::TSemanticFrameRequestData semanticFrame;
        if (!forRadio) {
            *(semanticFrame.MutableTypedSemanticFrame()->MutableMusicPlaySemanticFrame()) = NAlice::NMusic::ConstructMusicPlaySemanticFrame(
                object,
                repeat
            );
        } else {
            auto* fmRadioPlaySemanticFrame = semanticFrame.MutableTypedSemanticFrame()->MutableFmRadioPlaySemanticFrame();
            if (object.Has("fm_radio")) {
                fmRadioPlaySemanticFrame->MutableFmRadioStation()->SetFmRadioValue(object["fm_radio"].GetString());
            }
            if (object.Has("fm_radio_freq")) {
                fmRadioPlaySemanticFrame->MutableFmRadioFreq()->SetFmRadioFreqValue(object["fm_radio_freq"].GetString());
            }
        }

        auto* analytics = semanticFrame.MutableAnalytics();
        analytics->SetProductScenario(NAlice::NHollywood::NReminders::NScenarioNames::ALARM.Data());
        analytics->SetOrigin(NAlice::TAnalyticsTrackingModule_EOrigin_Scenario);
        analytics->SetPurpose(forRadio ? "start_alarm_with_radio" : "start_alarm_with_music");

        *(callback->MutablePayload()) = MessageToStruct(semanticFrame);
    }

    ctx.Renderer().AddDirective(std::move(directive));

    ctx.Renderer().AddVoiceCard(
        NAlice::NHollywood::NReminders::NNlgTemplateNames::ALARM_SET_SOUND,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );
}

bool IsAudioPlaying(const NAlice::TDeviceState& deviceState) {
    if (!deviceState.HasAudioPlayer()) {
        return false;
    }

    const auto& audioPlayerState = deviceState.GetAudioPlayer();
    return audioPlayerState.GetPlayerState() == NAlice::TDeviceState_TAudioPlayer_TPlayerState_Playing;
}

bool DoesAudioPlayerHaveTrackInfo(const NAlice::TDeviceState& deviceState) {
    if (!deviceState.HasAudioPlayer()) {
        return false;
    }

    const auto& currentStream = deviceState.GetAudioPlayer().GetCurrentlyPlaying();
    return !currentStream.GetStreamId().empty() &&
           !currentStream.GetTitle().empty() &&
           !currentStream.GetSubTitle().empty();
}

bool IsMusicPlaying(const NAlice::TDeviceState& deviceState) {
    const auto& music = deviceState.GetMusic();
    const auto& musicPlayer = music.GetPlayer();
    return musicPlayer.HasPause() && !musicPlayer.GetPause() && music.HasCurrentlyPlaying();
}


void SetAlarmAudioPlayerTrack(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const NAlice::TDeviceState::TAudioPlayer& audioPlayer,
    bool repeat
) {
    const auto& currentStream = audioPlayer.GetCurrentlyPlaying();

    NJson::TJsonValue info;
    info["type"] = "track";
    info["id"] = currentStream.GetStreamId();
    info["title"] = currentStream.GetTitle();

    info["artists"].AppendValue(
        NJson::TJsonMap({
            {"name", currentStream.GetSubTitle()}
        })
    );

    AddSetSoundCommand(
        ctx,
        info,
        repeat,
        /* forRadio = */ false
    );
}
bool DoesMusicPlayerHaveTrackInfo(const NAlice::TDeviceState& deviceState) {
    return deviceState.GetMusic().GetCurrentlyPlaying().GetRawTrackInfo().fields_size() != 0;
}

void SetAlarmMusicPlayerTrack(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const NAlice::TDeviceState::TMusic& musicPlayer,
    bool repeat
) {

    NJson::TJsonValue trackInfo = NAlice::JsonFromProto(musicPlayer.GetCurrentlyPlaying().GetRawTrackInfo());
    trackInfo["type"] = "track";

    auto musicInfo = NAlice::NMusic::ConvertMusicCatalogAnswerToMusicInfo(
        ctx.RunRequest().ClientInfo(),
        ctx.GetInterfaces().GetCanOpenLinkIntent(),
        /* needAutoplay = */ false,
        trackInfo
    );

    AddSetSoundCommand(
        ctx,
        musicInfo,
        repeat,
        /* forRadio = */ false
    );
}

} // namespace

namespace NAlice::NHollywood::NReminders {

void SetIrrelevant(TRemindersContext& ctx) {
    ctx.Renderer().SetIrrelevant();
    ConstructResponse(ctx);
    return;
}

void AlarmPlayAliceShow(
    TRemindersContext& ctx
) {
    auto& renderer = ctx.Renderer();
    if (!ctx.RunRequest().HasExpFlag(NExperiments::HW_ALARM_MORNING_SHOW)) {
        renderer.AddVoiceCard(
            NNlgTemplateNames::ALARM_MORNING_SHOW_ERROR
            , NNlgTemplateNames::NOT_SUPPORTED
        );

        renderer.SetIrrelevant();
        ConstructResponse(ctx);

        return;
    }

    if (const auto* callback = ctx.GetCallback(); callback) {
        renderer.SetProductScenarioName(NScenarioNames::MORNING_SHOW.Data());

        if (!ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
            RaiseErrorShowPromo(
                ctx,
                NNlgTemplateNames::ALARM_MORNING_SHOW_ERROR,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
            );

            return;
        }

        {
            NScenarios::TFrameAction action;
            TFrameNluHint* frameNluHint = action.MutableNluHint();
            frameNluHint->SetFrameName("next");

            {
                auto* instance = frameNluHint->AddInstances();
                instance->SetLanguage(ELang::L_RUS);
                instance->SetPhrase("хватит");
                FillParsedUtteranceForMorningShow(action.MutableParsedUtterance());
            }

            renderer.AddAction(
                "alarm_morning_show" // actionId
                , std::move(action)
            );
        }

        auto resetAddBuilder = renderer.ResetAddBuilder();
        FillParsedUtteranceForMorningShow(&resetAddBuilder.AddUtterance({}));

        renderer.AddVoiceCard(
            NNlgTemplateNames::ALARM_PLAY_ALICE_SHOW,
            NNlgTemplateNames::RENDER_RESULT
        );

        ConstructResponse(ctx);
        return;
    }

    renderer.AddVoiceCard(
        NNlgTemplateNames::ALARM_MORNING_SHOW_ERROR,
        NNlgTemplateNames::NOT_SUPPORTED
    );

    ConstructResponse(ctx);
    return;
}

void AlarmSetAliceShow(
    TRemindersContext& ctx,
    TStringBuf nlgTemplate,
    bool constructResponse
) {
    auto& renderer = ctx.Renderer();

    if (constructResponse) {
        SetAlarmProductScenario(ctx);
        renderer.SetIntentName(NFrameNames::ALARM_SET_ALICE_SHOW.Data());
    }
    if (!ctx.RunRequest().HasExpFlag(NExperiments::HW_ALARM_MORNING_SHOW)) {
        renderer.SetIrrelevant();

        renderer.AddAttention(ATTENTION_ALARM_MORNING_SHOW_NOT_SET);
        if (constructResponse) {
            renderer.AddVoiceCard(
                NNlgTemplateNames::ALARM_MORNING_SHOW_ERROR,
                NNlgTemplateNames::NOT_SUPPORTED
            );

            ConstructResponse(ctx);
        }

        return;
    }

    if (!AddAttentionIfSoundIsSupported(ctx)) {
        renderer.AddAttention(ATTENTION_ALARM_MORNING_SHOW_NOT_SET);
        if (constructResponse) {
            RaiseErrorShowPromo(
                ctx,
                NNlgTemplateNames::ALARM_MORNING_SHOW_ERROR,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
            );
        }

        return;
    }
    NScenarios::TDirective directive;
    auto* alarmSetSoundDirective = directive.MutableAlarmSetSoundDirective();
    auto* callback = alarmSetSoundDirective->MutableCallback();
    callback->SetName(NFrameNames::ALARM_PLAY_MORNING_SHOW.data());
    auto* payload = callback->MutablePayload();
    (*payload->mutable_fields())["name"].set_string_value(NFrameNames::ALARM_PLAY_MORNING_SHOW.data());
    alarmSetSoundDirective->MutableSettings()->SetType(TString(ALICE_SHOW_TYPE));

    renderer.AddDirective(std::move(directive));


    if (constructResponse) {
        renderer.AddVoiceCard(
            nlgTemplate,
            NNlgTemplateNames::RENDER_RESULT
        );

        ConstructResponse(ctx);
    }

    return;
}

void AlarmSetWithAliceShow(
    TRemindersContext& ctx
) {
    if (ctx.RunRequest().HasExpFlag(NExperiments::HW_ALARM_SET_WITH_MORNING_SHOW)) {
        AlarmSetAliceShow(ctx, NNlgTemplateNames::ALARM_SET_WITH_ALICE_SHOW, false /* constructResponse */);
        AlarmSet(ctx, NNlgTemplateNames::ALARM_SET_WITH_ALICE_SHOW, NFrameNames::ALARM_SET_WITH_ALICE_SHOW, false /* checkRelocationFlag */);
    } else {
        ctx.Renderer().AddAttention(ATTENTION_ALARM_SET_WITH_MORNING_SHOW_FALLBACK);
        AlarmSetAliceShow(ctx, NNlgTemplateNames::ALARM_SET_WITH_ALICE_SHOW, true /* constructResponse */);
    }
}

void AlarmWhatSoundIsSet(TRemindersContext& ctx) {
    auto& renderer = ctx.Renderer();
    const auto& soundAlarmSetting = ctx.GetAlarmState().GetSoundAlarmSetting();

    if (!AddAttentionIfSoundIsSupported(ctx)) {
        renderer.AddVoiceCard(
            NNlgTemplateNames::ALARM_WHAT_SOUND_IS_SET,
            NNlgTemplateNames::RENDER_RESULT
        );

        ConstructResponse(ctx);
        return;
    }

    AddAttentionIfNoAlarms(ctx);

    if (ctx.GetAlarmState().GetSoundAlarmSetting().GetType() == MORNING_SHOW_TYPE || // for backward compatibility
        ctx.GetAlarmState().GetSoundAlarmSetting().GetType() == ALICE_SHOW_TYPE) {
        NJson::TJsonValue cardData;
        cardData["alice_show_result"] = true;

        renderer.AddVoiceCard(
            NNlgTemplateNames::ALARM_WHAT_SOUND_IS_SET,
            NNlgTemplateNames::RENDER_RESULT,
            cardData
        );

        ConstructResponse(ctx);
        return;
    }

    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(NFrameNames::ALARM_WHAT_SOUND_IS_SET.Data());

    if (AddAttentionIfDefaultSoundIsSet(ctx)) {
        renderer.AddVoiceCard(
            NNlgTemplateNames::ALARM_WHAT_SOUND_IS_SET,
            NNlgTemplateNames::RENDER_RESULT
        );

        ConstructResponse(ctx);
        return;
    }

    const auto slot = soundAlarmSetting.GetType() == "radio" ? "radio_result" : "music_result";

    NJson::TJsonValue cardData;
    cardData[slot] = NAlice::JsonFromProto(soundAlarmSetting.GetRawInfo());

    renderer.AddVoiceCard(
        NNlgTemplateNames::ALARM_WHAT_SOUND_IS_SET,
        NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    return ConstructResponse(ctx);
}

void AlarmSoundSetLevel(TRemindersContext& ctx) {
    SetAlarmProductScenario(ctx);

    auto& renderer = ctx.Renderer();
    renderer.SetIntentName(NFrameNames::ALARM_SOUND_SET_LEVEL.Data());

    auto soundFrame = GetFrame(ctx.RunRequest().Input(), {NFrameNames::ALARM_SOUND_SET_LEVEL});
    if (ctx.GetInterfaces().GetCanChangeAlarmSoundLevel() && soundFrame) {
        if (!soundFrame->FindSlot("level")) {
            renderer.AddVoiceCard(
                NNlgTemplateNames::ALARM_SOUND_SET_LEVEL,
                NNlgTemplateNames::ASK_SOUND_LEVEL
            );

            return ConstructResponse(ctx);
        }

        const NSound::TDeviceVolume deviceSound(ctx.GetAlarmState().GetMaxSoundLevel());
        auto newSoundLevel = NSound::CalculateSoundLevelForSetLevel(*soundFrame, deviceSound);

        NAlice::NScenarios::TDirective directive;
        auto* resetSoundDirective = directive.MutableAlarmSetMaxLevelDirective();
        resetSoundDirective->SetName("alarm_set_max_level");

        if (!deviceSound.IsSupported(newSoundLevel)) {
            newSoundLevel = newSoundLevel > deviceSound.GetMax() ? deviceSound.GetMax() : deviceSound.GetMin();

            NJson::TJsonValue cardData;
            cardData.InsertValue("error_code", "level_out_of_range");
            renderer.AddVoiceCard(
                NNlgTemplateNames::ALARM_SOUND_SET_LEVEL,
                NNlgTemplateNames::RENDER_SOUND_ERROR,
                cardData
            );
        } else {
            NJson::TJsonValue cardData;
            cardData.InsertValue("level", newSoundLevel);
            renderer.AddVoiceCard(
                NNlgTemplateNames::ALARM_SOUND_SET_LEVEL,
                NNlgTemplateNames::RENDER_RESULT,
                cardData
            );
        }

        resetSoundDirective->SetNewLevel(newSoundLevel);
        renderer.AddDirective(std::move(directive));
        ConstructResponse(ctx);
    } else {
        RaiseErrorShowPromo(
            ctx,
            NNlgTemplateNames::ALARM_SOUND_SET_LEVEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
    }
}

void AlarmWhatSoundLevelIsSet(TRemindersContext& ctx) {
    SetAlarmProductScenario(ctx);
    auto& renderer = ctx.Renderer();
    renderer.SetIntentName(NFrameNames::ALARM_WHAT_SOUND_LEVEL_IS_SET.Data());

    if (!ClientSupportsAlarms(ctx)) {
        RaiseErrorShowPromo(
            ctx,
            NNlgTemplateNames::ALARM_WHAT_SOUND_LEVEL_IS_SET,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
    } else {
        constexpr i64 DEFAULT_MAX_SOUND_LEVEL = 7;

        i64 maxSoundLevel = ctx.GetAlarmState().GetMaxSoundLevel();
        if (maxSoundLevel <= 0) {
            maxSoundLevel = DEFAULT_MAX_SOUND_LEVEL;
        }

        NJson::TJsonValue cardData;
        cardData.InsertValue("level", maxSoundLevel);

        renderer.AddVoiceCard(
            NNlgTemplateNames::ALARM_WHAT_SOUND_LEVEL_IS_SET,
            NNlgTemplateNames::RENDER_RESULT,
            cardData
        );
        ConstructResponse(ctx);
    }
}

void AlarmSet(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& intent,
    bool /*checkRelocationFlag*/
) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(intent.Data());
    if (!ClientSupportsAlarms(ctx)) {
        RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
        return;
    }

    if (!ctx.RunRequest().Interfaces().GetHasReliableSpeakers()) {
        RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
        return;
    }
    const bool snoozeIntent = intent == NFrameNames::ALARM_SNOOZE_ABS || intent == NFrameNames::ALARM_SNOOZE_REL;

    if (snoozeIntent && ctx.GetAlarmState().GetCurrentlyPlaying()) {
        ctx.Renderer().AddAttention(ATTENTION_ALARM_SNOOZE);

        NScenarios::TDirective directive;
        auto* alarmStopDirective = directive.MutableAlarmStopDirective();
        alarmStopDirective->SetName("alarm_stop");

        ctx.Renderer().AddDirective(std::move(directive));
    }

    const TInstant now = GetCurrentTimestamp(ctx);
    NDatetime::TTimeZone tz;
    try {
        tz = NDatetime::GetTimeZone(ctx.GetTimezone());
    } catch (const NDatetime::TInvalidTimezone& e) {
        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NNlgTemplateNames::INVALID_TIME_ZONE
        );

        ConstructResponse(ctx);
        return ;
    }

    TVector<NScenarios::NAlarm::TWeekdaysAlarm> alarms;
    try {
        const auto alarmsState = GetAlarmsState(ctx);
        if (ctx.RunRequest().ClientInfo().IsSmartSpeaker() && alarmsState) {
            alarms = GetCalendarItems(ctx, tz, *alarmsState, now);
        }
    } catch (const NCalendarParser::TParser::TException& e) {
        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NNlgTemplateNames::BAD_ARGUMENTS
        );

        ConstructResponse(ctx);
        return ;
    }

    if (const auto& frame = TryGetCallbackUpdateFormFrame(ctx.RunRequest().Input().GetCallback()); frame) {
        const auto successSlot = frame->FindSlot(NSlots::SLOT_ALARM_SET_SUCCESS);
        if (!IsSlotEmpty(successSlot)) {
            AlarmSetConfirmation(ctx, frame, successSlot, nlgTemplate);
            return ConstructResponse(ctx);
        }
    }

    if (GetActiveAlarms(alarms, now).size() >= MAX_NUM_ALARMS) {
        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NNlgTemplateNames::TOO_MANY_ALARMS
        );

        ConstructResponse(ctx);
        return ;
    }

    auto frame = ctx.RunRequest().Input().CreateRequestFrame(intent);
    FillSlotsFromState(frame, ctx.State());
    ctx.Renderer().AddFrameSlots(frame);

    const auto timeSlot = frame.FindSlot(NSlots::SLOT_TIME);
    const auto dayPartSlot = frame.FindSlot(NSlots::SLOT_DAY_PART);
    const auto dateSlot = FindSlot(frame, NSlots::SLOT_DATE, NSlots::TYPE_DATE);
    auto weekdaysSlot = FindSlot(frame, NSlots::SLOT_DATE, NSlots::TYPE_WEEKDAYS);

    NJson::TJsonValue weekdays;
    if (!IsSlotEmpty(weekdaysSlot)) {
        NJson::ReadJsonFastTree(weekdaysSlot->Value.AsString(), &weekdays, false);
        if (weekdays["weekdays"].GetArray().ysize() == 1 && !weekdays["repeat"].GetBoolean()) {
            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NNlgTemplateNames::BAD_ARGUMENTS
            );

            ConstructResponse(ctx);
            return ;
        }
        weekdays["repeat"].SetValue(NJson::TJsonValue(true));
    }

    static const THashMap<TStringBuf, TStringBuf> dayPartToName = {
        {"night", "ночь"},
        {"nights", "ночь"},
        {"morning", "утро"},
        {"mornings", "утро"},
        {"day", "день"},
        {"evening", "вечер"},
        {"evenings", "вечер"}
    };

    if (IsSlotEmpty(timeSlot) && !snoozeIntent) {
        if (!IsSlotEmpty(dateSlot)) {
            const auto date = NScenarios::NAlarm::TDate::FromValue(NSc::TValue::FromJson(dateSlot->Value.AsString()));
            if (!IsTodayOrTomorrow(now, tz, date)) {
                ctx.Renderer().AddVoiceCard(
                    nlgTemplate,
                    NNlgTemplateNames::BAD_ARGUMENTS
                );

                ConstructResponse(ctx);
                return ;
            }
        }

        ctx.State().MutableSemanticFrame()->MergeFrom(frame.ToProto());

        NJson::TJsonValue cardData;
        TStringBuf phraseName = NNlgTemplateNames::ASK_TIME;

        if (!IsSlotEmpty(dayPartSlot)) {
            const auto* dayPartName = dayPartToName.FindPtr(dayPartSlot->Value.AsString());
            if (dayPartName && ctx.RunRequest().HasExpFlag("alarm_day_part")) {
                ctx.Renderer().AddAttention(ATTENTION_ALARM_ASK_TIME_FOR_DAY_PART);
                cardData["day_part_name"] = *dayPartName;
                phraseName = NNlgTemplateNames::ASK_CORRECTION_DAY_PART;
            }
        }

        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            phraseName,
            cardData
        );
        ctx.Renderer().SetShouldListen(true);

        AddAlarmShowSuggest(ctx);
        AddAlarmSetSuggests(ctx);

        ConstructResponse(ctx);
        return;
    }

    NJson::TJsonValue time;
    if (!IsSlotEmpty(timeSlot)) {
        NJson::ReadJsonFastTree(timeSlot->Value.AsString(), &time, false);
    }

    if (!IsSlotEmpty(dayPartSlot)) {
        if (!IsSlotEmpty(timeSlot)) {
            NAlice::NScenarios::NAlarm::AdjustTimeValue(time, dayPartSlot->Value.AsString());
            if (NAlice::NScenarios::NAlarm::IsPluralDayPart(dayPartSlot->Value.AsString())) {
                if (IsSlotEmpty(weekdaysSlot) && IsSlotEmpty(dateSlot)) {
                    if (weekdaysSlot) {
                        const_cast<TSlot*>(weekdaysSlot.Get())->Value = TSlot::TValue("{\"weekdays\":[1,2,3,4,5,6,7],\"repeat\":true}");
                    } else {
                        weekdaysSlot = AddSlot(frame, TSlot{NSlots::SLOT_DATE, NSlots::TYPE_WEEKDAYS, TSlot::TValue("{\"weekdays\":[1,2,3,4,5,6,7],\"repeat\":true}")});
                    }

                    NJson::ReadJsonFastTree(weekdaysSlot->Value.AsString(), &weekdays, false);
                }
            }
        }
    }

    auto dayTime = !IsSlotEmpty(timeSlot) ? NScenarios::NAlarm::TDayTime::FromValue(NSc::TValue::FromJsonValue(time))
                                          : NScenarios::NAlarm::TDayTime(Nothing() /* hours */,
                                                                         NScenarios::NAlarm::TDayTime::TComponent(10, true /* relative */) /* minutes */,
                                                                         Nothing() /* seconds */, NScenarios::NAlarm::TDayTime::EPeriod::Unspecified);

    if (!dayTime || dayTime->HasRelativeNegative()) {
        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NNlgTemplateNames::BAD_ARGUMENTS
        );

        ConstructResponse(ctx);
        return;
    }

    if (intent == NFrameNames::ALARM_SNOOZE_REL && !dayTime->IsRelative() && dayTime->Period == NScenarios::NAlarm::TDayTime::EPeriod::Unspecified) {
        static const auto setRelative = [](auto& comp) {
            if (comp) {
                comp->Relative = true;
            }
        };

        // Create relative version of dayTime
        NScenarios::NAlarm::TDayTime relDayTime = *dayTime;
        setRelative(relDayTime.Hours);
        setRelative(relDayTime.Minutes);
        setRelative(relDayTime.Seconds);

        // We use relative dayTime if it happens sooner than absolute
        const auto realNow = NDatetime::Convert(now, tz);
        if (GetAlarmTime(realNow, relDayTime) < GetAlarmTime(realNow, *dayTime)) {
            dayTime = relDayTime;
        }
    }

    if (!IsSlotEmpty(dateSlot) && !IsSlotEmpty(weekdaysSlot)) {
        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NNlgTemplateNames::BAD_ARGUMENTS
        );

        ConstructResponse(ctx);
        return;
    }

    NJson::TJsonValue cardData;
    if (IsSlotEmpty(dateSlot) && IsSlotEmpty(weekdaysSlot)) {
        NScenarios::NAlarm::TWeekdaysAlarm alarm;

        auto possibleAlarms = GetPossibleAlarms(now, tz, *dayTime);
        const auto alarmIdSlot = frame.FindSlot(NSlots::SLOT_ALARM_ID);

        ui32 index;
        if (IsSlotEmpty(alarmIdSlot)) {
            if (possibleAlarms.size() > 1) {
                ctx.State().MutableSemanticFrame()->MergeFrom(frame.ToProto());

                NJson::TJsonValue cardData;
                cardData["possible_alarms"] = ToJson({possibleAlarms}, NDatetime::Convert(now, tz), tz);

                ctx.Renderer().AddVoiceCard(
                    nlgTemplate,
                    NNlgTemplateNames::ASK_POSSIBLE_ALARMS,
                    cardData
                );
                ctx.Renderer().SetShouldListen(true);

                return ConstructResponse(ctx);
            } else {
                index = 1;
            }
        } else {
            index = JsonFromString(alarmIdSlot->Value.AsString()).GetUInteger();
        }

        if (index > possibleAlarms.size()) {
            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NNlgTemplateNames::BAD_ARGUMENTS
            );

            ConstructResponse(ctx);
            return;

        }

        // Need to decrement here because alarmId is usually "first",
        // "second", etc., i.e. 1-based.
        if (index != 0) {
            --index;
        }
        alarm = possibleAlarms[index].Alarm;

        if (!IsSlotEmpty(timeSlot)) {
            const_cast<TSlot*>(timeSlot.Get())->Value = TSlot::TValue(JsonToString(NScenarios::NAlarm::TimeToValue(alarm.Begin, tz).ToJsonValue()));
        }
        AddAlarmSetCommand(ctx, now, tz, alarms, alarm, frame, snoozeIntent);
        cardData["time"] = NScenarios::NAlarm::TimeToValue(alarm.Begin, tz).ToJsonValue();
    }

    if (!IsSlotEmpty(dateSlot)) {
        Y_ASSERT(IsSlotEmpty(weekdaysSlot));

        const auto date = NScenarios::NAlarm::TDate::FromValue(NSc::TValue::FromJson(dateSlot->Value.AsString()));
        if (!date || !date->HasExactDay()){
            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NNlgTemplateNames::BAD_ARGUMENTS
            );

            ConstructResponse(ctx);
            return;
        }

        const TMaybe<NScenarios::NAlarm::TWeekdaysAlarm> alarm = GetAlarmDateTime(now, tz, *dayTime, *date);
        if (!alarm) {
            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NNlgTemplateNames::BAD_ARGUMENTS
            );

            ConstructResponse(ctx);
            return;
        }

        if (!IsSlotEmpty(timeSlot)) {
            const_cast<TSlot*>(timeSlot.Get())->Value = TSlot::TValue(JsonToString(NScenarios::NAlarm::TimeToValue(alarm->Begin, tz).ToJsonValue()));
        }

        AddAlarmSetCommand(ctx, now, tz, alarms, *alarm, frame);
        cardData["time"] = NScenarios::NAlarm::TimeToValue(alarm->Begin, tz).ToJsonValue();
    }

    if (!IsSlotEmpty(weekdaysSlot)) {
        Y_ASSERT(IsSlotEmpty(dateSlot));
        const auto ws = NScenarios::NAlarm::TWeekdays::FromValue(NSc::TValue::FromJsonValue(weekdays));
        if (!ws) {
            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NNlgTemplateNames::BAD_ARGUMENTS
            );

            ConstructResponse(ctx);
            return;
        }

        Y_ASSERT(ws->Repeat);

        const NScenarios::NAlarm::TWeekdaysAlarm alarm = GetAlarmWeekdays(now, tz, *dayTime, *ws);
        if (!IsSlotEmpty(timeSlot)) {
            const_cast<TSlot*>(timeSlot.Get())->Value = TSlot::TValue(JsonToString(NScenarios::NAlarm::TimeToValue(alarm.Begin, tz).ToJsonValue()));
        }
        AddAlarmSetCommand(ctx, now, tz, alarms, alarm, frame);
        cardData["time"] = NScenarios::NAlarm::TimeToValue(alarm.Begin, tz).ToJsonValue();
    }

    ctx.Renderer().AddVoiceCard(
        nlgTemplate,
        NNlgTemplateNames::RENDER_RESULT,
        cardData
    );
    ConstructResponse(ctx);
}

void AlarmShow(TRemindersContext& ctx) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(NFrameNames::ALARM_SHOW.Data());
    if (!ClientSupportsAlarms(ctx)) {
        RaiseErrorShowPromo(
            ctx,
            NNlgTemplateNames::ALARM_SHOW,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
        return;
    }

    const TInstant now = GetCurrentTimestamp(ctx);
    NDatetime::TTimeZone tz;
    try {
        tz = NDatetime::GetTimeZone(ctx.GetTimezone());
    } catch (const NDatetime::TInvalidTimezone& e) {
        ctx.Renderer().AddVoiceCard(
            NNlgTemplateNames::ALARM_SHOW,
            NNlgTemplateNames::INVALID_TIME_ZONE
        );

        ConstructResponse(ctx);
        return ;
    }

    if (ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
        TVector<NScenarios::NAlarm::TWeekdaysAlarm> alarms;
        const auto alarmsState = GetAlarmsState(ctx);
        if (alarmsState) {
            alarms = GetCalendarItems(ctx, tz, *alarmsState, now);
        }

        auto activeAlarms = GetActiveAlarms(alarms, now);

        auto frame = ctx.RunRequest().Input().CreateRequestFrame(NFrameNames::ALARM_SHOW);

        const auto timeSlot = frame.FindSlot(NSlots::SLOT_TIME);
        const auto dateSlot = FindSlot(frame, NSlots::SLOT_DATE, NSlots::TYPE_DATE);
        const auto weekdaysSlot = FindSlot(frame, NSlots::SLOT_DATE, NSlots::TYPE_WEEKDAYS);

        TMaybe<NScenarios::NAlarm::TDayTime> time;
        TMaybe<NScenarios::NAlarm::TDate> date;
        TMaybe<NScenarios::NAlarm::TWeekdays> weekdays;

        if (!FromSlot(timeSlot, time) || !FromSlot(dateSlot, date) || !FromSlot(weekdaysSlot, weekdays)) {
            RaiseErrorShowPromo(
                ctx,
                NNlgTemplateNames::ALARM_SHOW,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
            );
            return;
        }

        if (weekdays) {
            activeAlarms = FilterApproximateMatchingDayTimeAndWeekdays(activeAlarms, time, *weekdays, NDatetime::Convert(now, tz), tz);
        } else {
            activeAlarms = FilterApproximateMatchingDayTimeAndDate(activeAlarms, time, date, NDatetime::Convert(now, tz), tz);
        }

        if (activeAlarms.empty()) {
            NJson::TJsonValue cardData;
            if (time) {
                cardData["time"] = true;
            }

            if (weekdays) {
                cardData["weekdays"] = true;
            }

            if (date) {
                cardData["date"] = true;
            }

            ctx.Renderer().AddVoiceCard(
                NNlgTemplateNames::ALARM_SHOW,
                NNlgTemplateNames::NO_ALARMS,
                cardData
            );

            ConstructResponse(ctx);
            return ;
        }

        Sort(activeAlarms.begin(), activeAlarms.end());

        auto availableAlarmsSlot = FindOrAddSlot(
            frame,
            NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_ALARMS,
            NAlice::NHollywood::NReminders::NSlots::TYPE_LIST
        );

        const_cast<TSlot*>(availableAlarmsSlot.Get())->Value = NAlice::NHollywood::TSlot::TValue(
            NAlice::JsonToString(ToJson(activeAlarms, NDatetime::Convert(now, tz), tz))
        );
        ctx.State().MutableSemanticFrame()->MergeFrom(frame.ToProto());


        NJson::TJsonValue cardData;
        cardData["available_alarms"] = ToJson(activeAlarms, NDatetime::Convert(now, tz), tz);
        ctx.Renderer().AddVoiceCard(
            NNlgTemplateNames::ALARM_SHOW,
            NNlgTemplateNames::RENDER_RESULT,
            cardData
        );
        ctx.Renderer().SetShouldListen(true);

        ConstructResponse(ctx);
        return;
    }

    AddAlarmShowSuggest(ctx);
    AddAlarmSetSuggests(ctx);

    NScenarios::TDirective directive;
    auto* showAlarmsDirective = directive.MutableShowAlarmsDirective();
    showAlarmsDirective->SetName("show_alarms");
    ctx.Renderer().AddDirective(std::move(directive));

    ctx.Renderer().AddAttention(ATTENTION_ALARM_IS_ANDROID);

    ConstructResponse(ctx);
    return;
}

void AlarmStopPlaying(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName
) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(frameName.Data());

    if (!ClientSupportsAlarms(ctx)) {
        ctx.Renderer().SetIrrelevant();

        RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
        return;
    }

    if (!ctx.GetDeviceState().GetAlarmState().GetCurrentlyPlaying()) {
        return SetIrrelevant(ctx);
    }

    NScenarios::TDirective directive;
    auto* alarmStopPlayingDirective = directive.MutableAlarmStopDirective();
    alarmStopPlayingDirective->SetName("alarm_stop_playing");
    ctx.Renderer().AddDirective(std::move(directive));

    ctx.Renderer().AddVoiceCard(
        nlgTemplate,
        NNlgTemplateNames::RENDER_RESULT
    );

    return ConstructResponse(ctx);
}

void AlarmHowLong(TRemindersContext& ctx) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(NFrameNames::ALARM_HOW_LONG.Data());
    if (!ClientSupportsAlarms(ctx)) {
        RaiseErrorShowPromo(
            ctx,
            NNlgTemplateNames::ALARM_HOW_LONG,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
        return;
    }

    const TInstant now = GetCurrentTimestamp(ctx);
    NDatetime::TTimeZone tz;
    try {
        tz = NDatetime::GetTimeZone(ctx.GetTimezone());
    } catch (const NDatetime::TInvalidTimezone& e) {
        ctx.Renderer().AddVoiceCard(
            NNlgTemplateNames::ALARM_HOW_LONG,
            NNlgTemplateNames::INVALID_TIME_ZONE
        );

        ConstructResponse(ctx);
        return ;
    }

    TVector<NScenarios::NAlarm::TWeekdaysAlarm> alarms;
    const auto alarmsState = GetAlarmsState(ctx);
    if (alarmsState) {
        alarms = GetCalendarItems(ctx, tz, *alarmsState, now);
    }

    auto activeAlarms = GetActiveAlarms(alarms, now);

    if (activeAlarms.empty()) {
        ctx.Renderer().AddVoiceCard(
            NNlgTemplateNames::ALARM_HOW_LONG,
            NNlgTemplateNames::NO_ALARMS
        );

        ConstructResponse(ctx);
        return ;
    }

    NScenarios::NAlarm::TDayTime answer(24, 60, 0, NScenarios::NAlarm::TDayTime::EPeriod::Unspecified);
    NDatetime::TCivilSecond conv = NDatetime::Convert(now, tz);

    NScenarios::NAlarm::TDayTime nowTime(conv.hour(), conv.minute(), conv.second(), NScenarios::NAlarm::TDayTime::EPeriod::Unspecified);
    bool hasAlarmInNearestFuture = false;
    int nowDay = static_cast<int>(NScenarios::NAlarm::GetWeekday(conv));

    for (auto al : activeAlarms) {
        bool isAlarmInNearestFuture = false;

        auto beg = al.Alarm.Begin;
        conv = NDatetime::Convert(beg, tz);

        NScenarios::NAlarm::TDayTime alarmTime(conv.hour(), conv.minute(), conv.second(), NScenarios::NAlarm::TDayTime::EPeriod::Unspecified);

        bool todayAlarm = IsFirstBeforeSecond(nowTime, alarmTime);
        if (al.Alarm.Weekdays) {
            for (auto d: al.Alarm.Weekdays->Days) {
                int numDay = static_cast<int>(d);
                int comp = (numDay + 7 - nowDay) % 7;
                if (comp == 1 && !todayAlarm || comp == 0 && todayAlarm) {
                    isAlarmInNearestFuture = true;
                }
            }
        } else {
            isAlarmInNearestFuture = true;
        }

        if (!isAlarmInNearestFuture) {
            continue;
        }

        hasAlarmInNearestFuture = true;

        NScenarios::NAlarm::TDayTime diffTime;
        if (!todayAlarm) {
            alarmTime.Hours->Value += 24;
        }

        GetDistInTime(nowTime, alarmTime, diffTime);
        if (IsFirstBeforeSecond(diffTime, answer)){
            answer = diffTime;
        }
    }

    if (!hasAlarmInNearestFuture) {
        ctx.Renderer().AddVoiceCard(
            NNlgTemplateNames::ALARM_HOW_LONG,
            NNlgTemplateNames::NO_ALARMS_IN_NEAREST_FUTURE
        );

        ConstructResponse(ctx);
        return;
    }

    NJson::TJsonValue cardData;
    cardData["how_long"] = answer.ToValue().ToJsonValue();
    ctx.Renderer().AddVoiceCard(
        NNlgTemplateNames::ALARM_HOW_LONG,
        NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    ConstructResponse(ctx);
}

void AlarmResetSound(TRemindersContext& ctx) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(NFrameNames::ALARM_RESET_SOUND.Data());
    if (AddAttentionIfSoundIsSupported(ctx) && !AddAttentionIfDefaultSoundIsSet(ctx)) {
        NScenarios::TDirective directive;
        auto* alarmResetSoundDirective = directive.MutableAlarmResetSoundDirective();
        alarmResetSoundDirective->SetName("alarm_reset_sound");
        ctx.Renderer().AddDirective(std::move(directive));
    }

    ctx.Renderer().AddVoiceCard(
        NNlgTemplateNames::ALARM_RESET_SOUND,
        NNlgTemplateNames::RENDER_RESULT
    );

    ConstructResponse(ctx);

    return;
}

void AlarmCancel(
    TRemindersContext& ctx,
    const TStringBuf intent
) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(intent.Data());

    if (!ClientSupportsAlarms(ctx)) {
        RaiseErrorShowPromo(
            ctx,
            NNlgTemplateNames::ALARM_CANCEL,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
        );
        return;
    }

    const TInstant now = GetCurrentTimestamp(ctx);
    NDatetime::TTimeZone tz;
    try {
        tz = NDatetime::GetTimeZone(ctx.GetTimezone());
    } catch (const NDatetime::TInvalidTimezone& e) {
        ctx.Renderer().AddVoiceCard(
            NNlgTemplateNames::ALARM_CANCEL,
            NNlgTemplateNames::INVALID_TIME_ZONE
        );

        return ConstructResponse(ctx);
    }

    if (ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
        TVector<NScenarios::NAlarm::TWeekdaysAlarm> alarms;
        const auto alarmsState = GetAlarmsState(ctx);
        if (alarmsState) {
            alarms = GetCalendarItems(ctx, tz, *alarmsState, now);
        }

        auto activeAlarms = GetActiveAlarms(alarms, now);

        if (activeAlarms.empty()) {
            ctx.Renderer().AddVoiceCard(
                NNlgTemplateNames::ALARM_CANCEL,
                NNlgTemplateNames::NO_ALARMS
            );
            return ConstructResponse(ctx);
        }

        auto frame = ctx.RunRequest().Input().CreateRequestFrame(intent);
        FillSlotsFromState(frame, ctx.State());

        const auto alarmIdSlot = FindSlot(frame, NSlots::SLOT_ALARM_ID, NSlots::TYPE_NUM);

        if (!IsSlotEmpty(alarmIdSlot)) {
            return CancelById(
                ctx,
                frame,
                alarms,
                NDatetime::Convert(now, tz),
                tz,
                JsonFromString(alarmIdSlot->Value.AsString())
            );
        }

        for (const TString& type : {NSlots::TYPE_SELECTION_DEPRECATED, NSlots::TYPE_SELECTION}) {
            const auto alarmSelectionSlot = FindSlot(frame, NSlots::SLOT_ALARM_ID, type);
            if (!IsSlotEmpty(alarmSelectionSlot)) {
                return CancelBySelection(
                    ctx,
                    frame,
                    alarms,
                    activeAlarms,
                    NDatetime::Convert(now, tz),
                    tz,
                    JsonFromString(alarmSelectionSlot->Value.AsString())
                );
            }
        }

        const auto timeSlot = FindSlot(frame, NSlots::SLOT_TIME, NSlots::TYPE_TIME);
        const auto dateSlot = FindSlot(frame, NSlots::SLOT_DATE, NSlots::TYPE_DATE);
        const auto weekdaysSlot = FindSlot(frame, NSlots::SLOT_DATE, NSlots::TYPE_WEEKDAYS);

        if (!IsSlotEmpty(dateSlot) && !IsSlotEmpty(weekdaysSlot)) {
            ctx.Renderer().AddVoiceCard(
                NNlgTemplateNames::ALARM_RESET_SOUND,
                NNlgTemplateNames::BAD_ARGUMENTS
            );

            return ConstructResponse(ctx);
        }

        TMaybe<NScenarios::NAlarm::TDayTime> dayTime;
        TMaybe<NScenarios::NAlarm::TDate> date;
        TMaybe<NScenarios::NAlarm::TWeekdays> weekdays;
        if (!FromSlot(timeSlot, dayTime) || !FromSlot(dateSlot, date) || !FromSlot(weekdaysSlot, weekdays)) {
            ctx.Renderer().AddVoiceCard(
                NNlgTemplateNames::ALARM_CANCEL,
                NNlgTemplateNames::BAD_ARGUMENTS
            );

            return ConstructResponse(ctx);
        }

        Y_ASSERT(!activeAlarms.empty());
        if (activeAlarms.size() == 1) {
            if (weekdays) {
                return CancelRegular1Set(
                    ctx,
                    frame,
                    alarms,
                    activeAlarms[0],
                    NDatetime::Convert(now, tz),
                    tz,
                    dayTime,
                    *weekdays
                );
            }

            return CancelSingle1Set(
                ctx,
                frame,
                alarms,
                activeAlarms[0],
                NDatetime::Convert(now, tz),
                tz,
                dayTime,
                date
            );
        }

        if (weekdays) {
            return CancelRegularManySet(
                ctx,
                frame,
                alarms,
                activeAlarms,
                NDatetime::Convert(now, tz),
                tz,
                dayTime,
                *weekdays
            );
        }

        return CancelSingleManySet(
            ctx,
            frame,
            alarms,
            activeAlarms,
            NDatetime::Convert(now, tz),
            tz,
            dayTime,
            date
        );
    }

    AddAlarmShowSuggest(ctx);
    AddAlarmSetSuggests(ctx);

    NScenarios::TDirective directive;
    auto* showAlarmsDirective = directive.MutableShowAlarmsDirective();
    showAlarmsDirective->SetName("show_alarms");
    ctx.Renderer().AddDirective(std::move(directive));

    ctx.Renderer().AddAttention(ATTENTION_ALARM_IS_ANDROID);

    return ConstructResponse(ctx);
}

void AlarmPrepareSetSound(
    TRemindersContext& ctx,
    const TStringBuf& frameName
) {
    auto frame = ctx.RunRequest().Input().CreateRequestFrame(frameName);

    const auto musicSearchSlot = frame.FindSlot(NSlots::SLOT_MUSIC_SEARCH);
    const auto playlistSlot = frame.FindSlot(NSlots::SLOT_PLAYLIST);

    if (!IsSlotEmpty(musicSearchSlot) || !IsSlotEmpty(playlistSlot)) {
        if (!CanPlayMusic(ctx)) {
            return;
        }

        TString encodedAliceMeta;

        frame.AddSlot(TSlot{.Name=NSlots::SLOT_SEARCH_TEXT.Data(), .Type=musicSearchSlot->Type, .Value=musicSearchSlot->Value});

        auto webRequestBuilder = NAlice::NHollywood::NMusic::ConstructSearchRequest(
            ctx.Ctx().Ctx.ScenarioResources<NMusic::TMusicResources>(),
            frame,
            ctx.RunRequest(),
            ctx.GetInterfaces(),
            ctx.Ctx().RequestMeta,
            ctx.Logger(),
            encodedAliceMeta,
            false
        );

        NAppHostRequest::TAppHostHttpProxyRequestBuilder searchHttpRequestBuilder;
        webRequestBuilder.Flush(searchHttpRequestBuilder);

        auto searchHttpRequest = searchHttpRequestBuilder.CreateRequest();

        ctx->ServiceCtx.AddProtobufItem(searchHttpRequest, NMusic::MUSIC_SEARCH_HTTP_REQUEST_ITEM);
    }
}

void AlarmSetSound(
    TRemindersContext& ctx,
    const TStringBuf frameName
) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(frameName.Data());

    if (!AddAttentionIfSoundIsSupported(ctx)) {
        ctx.Renderer().AddVoiceCard(
            NNlgTemplateNames::ALARM_SET_SOUND,
            NNlgTemplateNames::RENDER_RESULT
        );

        return ConstructResponse(ctx);
    }

    AddAttentionIfNoAlarms(ctx);

    auto frame = ctx.RunRequest().Input().CreateRequestFrame(frameName);

    const auto fmRadioSlot = frame.FindSlot(NSlots::SLOT_FM_RADIO);
    const auto fmRadioFreqSlot = frame.FindSlot(NSlots::SLOT_FM_RADIO_FREQ);
    const bool isFmRadio = fmRadioSlot && fmRadioSlot->Type == "custom.fm_radio_station";
    const bool isFmRadioFreq = fmRadioFreqSlot && fmRadioFreqSlot->Type == "custom.fm_radio_freq";
    LOG_INFO(ctx.Logger()) << "isFmRadio = " << isFmRadio << ", isFmRadioFreq = " << isFmRadioFreq;

    if (ctx.RunRequest().HasExpFlag(NExperiments::HW_ALARM_RELOCATION__ALARM_SET_SOUND_RADIO)) {
        if (isFmRadio || isFmRadioFreq) {
            const TMaybe<TString> fmRadioName = GetFmRadioName(
                ctx.RunRequest(),
                isFmRadio,
                fmRadioSlot,
                fmRadioFreqSlot,
                ctx.Ctx().Ctx.ScenarioResources<NMusic::TMusicResources>().GetFmRadioResources()
            );

            const TMaybe<TString> fmRadioId = GetFmRadioId(
                fmRadioName,
                ctx.Ctx().Ctx.ScenarioResources<NMusic::TMusicResources>().GetFmRadioResources()
            );

            if (fmRadioId) {
                NJson::TJsonValue radioInfo;

                if (isFmRadio) {
                    radioInfo["title"] = fmRadioSlot->Value.AsString();
                    radioInfo["fm_radio"] = *fmRadioId;
                } else {
                    radioInfo["title"] = fmRadioSlot->Value.AsString();
                    radioInfo["fm_radio_freq"] = fmRadioFreqSlot->Value.AsString();
                }

                AddSetSoundCommand(
                    ctx,
                    radioInfo,
                    /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT)),
                    /* forRadio = */ true
                );

                return ConstructResponse(ctx);
            } else {
                ctx.Renderer().AddAttention(ATTENTION_ALARM_RADIO_SEARCH_FAILURE);

                ctx.Renderer().AddVoiceCard(
                    NNlgTemplateNames::ALARM_SET_SOUND,
                    NNlgTemplateNames::RENDER_RESULT
                );

                return ConstructResponse(ctx);
            }
        }
    }

    const auto musicSearchSlot = frame.FindSlot(NSlots::SLOT_MUSIC_SEARCH);
    const auto playlistSlot = frame.FindSlot(NSlots::SLOT_PLAYLIST);

    if (!IsSlotEmpty(musicSearchSlot) || !IsSlotEmpty(playlistSlot)) {
        if (auto responseJsonStr = NMusic::GetRawHttpResponseMaybe(ctx.Ctx(), NMusic::MUSIC_CATALOG_HTTP_RESPONSE_ITEM)) {
            if (!CanPlayMusic(ctx)) {
                ctx.Renderer().AddVoiceCard(
                    NNlgTemplateNames::ALARM_SET_SOUND,
                    NNlgTemplateNames::RENDER_RESULT
                );

                return ConstructResponse(ctx);
            }

            bool isFairyTaleFilterGenre = IsFairyTaleFilterGenre(frame);

            auto musicCatalogResponse = NMusic::ParseMusicCatalogResponse(
                ctx.RunRequest().ClientInfo(),
                NAlice::JsonFromString(*responseJsonStr),
                ExpFlagsFromProto(ctx.RunRequest().Proto().GetBaseRequest().GetExperiments()),
                ctx.GetInterfaces().GetCanOpenLinkIntent(),
                isFairyTaleFilterGenre,
                ctx.Logger()
            );


            AddSetSoundCommand(
                ctx,
                musicCatalogResponse,
                /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT)),
                /* forRadio = */ false
            );

            return ConstructResponse(ctx);
        }
    }

    const auto thisSlot = frame.FindSlot(NSlots::SLOT_THIS);

    if (IsSlotEmpty(thisSlot)) {
        NJson::TJsonValue filters;
        for (TStringBuf slotName : NSlots::MUSIC_FILTER_SLOTS) {
            if (const auto slot = frame.FindSlot(slotName); !IsSlotEmpty(slot)) {
                filters["filters"][slotName] = slot->Value.AsString();
            }
        }

        if (filters.Has("filters") && !filters["filters"].GetMap().empty()) {
            if (!CanPlayMusic(ctx)) {
                ctx.Renderer().AddVoiceCard(
                    NNlgTemplateNames::ALARM_SET_SOUND,
                    NNlgTemplateNames::RENDER_RESULT
                );

                return ConstructResponse(ctx);
            }

            AddSetSoundCommand(
                ctx,
                filters,
                /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT)),
                /* forRadio = */ false
            );

            return ConstructResponse(ctx);
        }

        return SetIrrelevant(ctx);
    } else {
        auto deviceState = ctx.GetDeviceState();

        auto screen = deviceState.GetVideo().GetCurrentScreen();
        const bool showingOnTv = deviceState.GetIsTvPluggedIn() && screen;

        if (IsMusicPlaying(deviceState) && DoesMusicPlayerHaveTrackInfo(deviceState)) {
            SetAlarmMusicPlayerTrack(
                ctx,
                deviceState.GetMusic(),
                /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT))
            );
        } else if (IsAudioPlaying(deviceState) && DoesAudioPlayerHaveTrackInfo(deviceState)) {
            SetAlarmAudioPlayerTrack(
                ctx,
                deviceState.GetAudioPlayer(),
                /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT))
            );
        } else if (showingOnTv && screen == "music_player") {
            if (DoesMusicPlayerHaveTrackInfo(deviceState)) {
                const auto& musicPlayer = deviceState.GetMusic();
                if (DoesAudioPlayerHaveTrackInfo(deviceState)) {
                    const auto& audioPlayer = deviceState.GetAudioPlayer();

                    if (musicPlayer.GetLastPlayTimestamp() >= audioPlayer.GetLastPlayTimestamp()) {
                        SetAlarmMusicPlayerTrack(
                            ctx,
                            deviceState.GetMusic(),
                            /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT))
                        );
                    } else {
                        SetAlarmAudioPlayerTrack(
                            ctx,
                            deviceState.GetAudioPlayer(),
                            /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT))
                        );
                    }
                } else {
                    SetAlarmMusicPlayerTrack(
                        ctx,
                        deviceState.GetMusic(),
                        /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT))
                    );
                }
            } else if (DoesAudioPlayerHaveTrackInfo(deviceState)) {
                SetAlarmAudioPlayerTrack(
                    ctx,
                    deviceState.GetAudioPlayer(),
                    /* repeat = */ !IsSlotEmpty(frame.FindSlot(NSlots::SLOT_REPEAT))
                );
            } else {
                return SetIrrelevant(ctx);
            }
        } else {
            return SetIrrelevant(ctx);
        }
        return ConstructResponse(ctx);
    }
}

void AlarmHowToSetSound(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName
) {
    SetAlarmProductScenario(ctx);
    ctx.Renderer().SetIntentName(frameName.Data());

    AddAttentionIfSoundIsSupported(ctx);

    ctx.Renderer().AddVoiceCard(
        nlgTemplate,
        NNlgTemplateNames::RENDER_RESULT
    );

    return ConstructResponse(ctx);
}

void AlarmFallback(TRemindersContext& ctx) {
    if (!CheckExpFlag(ctx, NExperiments::HW_ALARM_RELOCATION__FALLBACK, NNlgTemplateNames::ALARM_FALLBACK)) {
        return;
    }

    ctx.Renderer().AddVoiceCard(
        NNlgTemplateNames::ALARM_FALLBACK,
        NNlgTemplateNames::RENDER_RESULT
    );

    return ConstructResponse(ctx);
}

} // namespace NAlice::NHollywood::NReminders
