#include "timer.h"

#include "helpers.h"

#include <alice/library/scenarios/alarm/helpers.h>

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/iterator/mapped.h>
#include <library/cpp/iterator/zip.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/scope.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <utility>

namespace NBASS {
namespace NReminders {

namespace {

constexpr TStringBuf DIRECTIVE_NAME_KEY = "name";

constexpr TStringBuf ERROR_CODE_UNSUPPORTED = "unsupported_operation";
constexpr TStringBuf ERROR_CODE_NOTIMERS = "no_timers_available";
constexpr TStringBuf ERROR_CODE_BADARGS = "bad_arguments";
constexpr TStringBuf ERROR_CODE_DURATION_TOO_LONG = "bad_arguments";
constexpr TStringBuf ERROR_CODE_TOO_MANY_TIMERS = "too_many_timers";
constexpr TStringBuf ERROR_CODE_INVALID_TIME = "invalid_time";

constexpr TStringBuf ATTBLOCK_ABS_TIME = "timer__abs_time";
constexpr TStringBuf ATTBLOCK_IS_MOBILE = "timer__is_mobile";
constexpr TStringBuf ATTBLOCK_MULTIPLE_TIMERS = "timer__multiple_timers_for_time";
constexpr TStringBuf ATTBLOCK_NOTIMERS_FOR = "timer__no_timers_for_time";
constexpr TStringBuf ATTBLOCK_IS_PAUSED = "timer__is_paused";

constexpr TStringBuf SLOT_NAME_AVAILTIMERS = "available_timers";
constexpr TStringBuf SLOT_NAME_TIMERID = "timer_id";
constexpr TStringBuf SLOT_NAME_CONFIRMATION = "confirmation";
constexpr TStringBuf SLOT_TYPE_CONFIRMATION = "bool";
constexpr TStringBuf SLOT_NAME_SPECIFICATION = "specification";
constexpr TStringBuf SLOT_TYPE_SPECIFICATION = "timer_specification";
constexpr TStringBuf SLOT_NAME_TIME = "time";
constexpr TStringBuf SLOT_TYPE_TIMEUNITS = "units_time";
constexpr TStringBuf SLOT_TYPE_AVAILTIMERS = "list";
constexpr TStringBuf SLOT_NAME_NOW = "now";
constexpr TStringBuf SLOT_TYPE_NOW = "string";

constexpr TStringBuf ACTION_SHOW_TIMERS = "show_timers";
constexpr TStringBuf ACTION_SET_TIMER = "set_timer";

constexpr TDuration MAX_DURATION_ALLOWED = TDuration::Days(1);

constexpr i64 SUGGESTED_MINUTES[] = { 10, 5, 1 };

constexpr size_t MAX_TIMERS_ON_QUASAR = 25;

const NSc::TValue SLEEP_DIRECTIVES = NSc::TValue::FromJson(TStringBuf(R"(
[
    {
        "name": "player_pause",
        "type": "client_action",
        "payload": {
            "smooth": true
        }
    },
    {
        "name": "clear_queue",
        "type": "client_action"
    },
    {
        "name": "go_home",
        "type": "client_action"
    },
    {
        "name": "screen_off",
        "type": "client_action"
    }
]
)"));

class TTimer;
using TTimerVal = TMaybe<TTimer>;

class TTimer {
public:
    using TScheme = TContext::TDeviceState::TTimerConst;
    using TList = TVector<TScheme>;

public:
    explicit TTimer(TDuration duration, ETimerType timerType = ETimerType::Normal);
    TTimer(TInstant timestamp, const NSc::TValue& timeValue, ETimerType timerType = ETimerType::Normal);

    /** Insert normalized time_* values into json
     * @param[out] json is an output json
     */
    void ToJson(NSc::TValue* json) const;

    /** Overwrite (or create new) slot "time"/"units_time" with normalized values of current timer
     */
    void UpdateForm(TContext& ctx) const;

    /** Copy non zero "time_*" slots from source context to destination
     */
    void CopySlots(const TContext& from, TContext* to);

    /** Try to obtain timer from slots.
     * In case of error it adds an error block into context.
     * @param[in|out] ctx is the context from which it gets slots and where it puts error blocks
     * @param[out] timer is a place where to put found timer (or not if requested slots not found) even if error is occured
     * @return true in case there is no error, otherwise false
     */
    static bool FromSlots(TContext& ctx, TTimerVal* timer);

    static void ToJson(TTimer::TScheme timerScheme, TContext& ctx, NSc::TValue* json);
    static void UpdateForm(const NSc::TValue& timer, TContext& ctx);
    static void UpdateForm(TTimer::TScheme timerScheme, TContext& ctx);

    /** Get timers from device state and sort it
     */
    static TList GetDeviceTimers(const TContext& ctx);

    const TDuration Duration;
    TMaybe<TInstant> Timestamp;

private:
    static TSlot* CreateTimeSlot(TContext& ctx);
    static TSlot* CreateTimeUnitsSlot(TContext& ctx);

    static bool FromTimeUnitsSlot(TContext& ctx, TTimerVal* timer);
    static bool FromTimeSlot(TContext& ctx, TTimerVal* timer);


private:
    struct STimeSource {
        const TStringBuf UnitName;
        const TDuration Multiplier;
        const std::function<ui64(ui64)> Init;
    };
    static const TVector<STimeSource> Sources;

    TVector<std::pair<const STimeSource&, ui16>> Data;
    TMaybe<NSc::TValue> TimeValue;
    ETimerType Type;
};

const TVector<TTimer::STimeSource> TTimer::Sources = {
    { "hours", TDuration::Hours(1),   [](ui64 duration) { return duration / 3600; } },
    { "minutes", TDuration::Minutes(1), [](ui64 duration) { return (duration % 3600) / 60; } },
    { "seconds", TDuration::Seconds(1), [](ui64 duration) { return (duration % 3600) % 60; } },
};

class TSplittedTimers {
public:
    TSplittedTimers(const TContext& ctx, ui64 duration)
        : Device{TTimer::GetDeviceTimers(ctx)}
    {
        ActiveTimers.reserve(Device.size());
        for (const auto& timer : Device) {
            if (timer.Duration() == duration) {
                if (timer.IsPaused()) {
                    PausedTimers.emplace_back(timer);
                } else {
                    ActiveTimers.emplace_back(timer);
                }
            }
        }
    }

    const TTimer::TList& Active() const {
        return ActiveTimers;
    }

    const TTimer::TList& Paused() const {
        return PausedTimers;
    }

    const TTimer::TList Device;

private:
    TTimer::TList ActiveTimers;
    TTimer::TList PausedTimers;
};

bool IsSleepTimerRequest(const TContext& ctx) {
    const auto* specificationSlot = ctx.GetSlot(SLOT_NAME_SPECIFICATION, SLOT_TYPE_SPECIFICATION);
    return !IsSlotEmpty(specificationSlot) && specificationSlot->Value == ToString(ETimerType::Sleep);
}

ETimerType InferTimerTypeFromCtx(const TContext& ctx) {
    return IsSleepTimerRequest(ctx) ? ETimerType::Sleep : ETimerType::Normal;
}

void UpdateSpecificationSlot(TContext& ctx, const ETimerType timerType) {
    ctx.CreateSlot(SLOT_NAME_SPECIFICATION, SLOT_TYPE_SPECIFICATION, true /* optional */, NSc::TValue(ToString(timerType)));
}

TContext::TBlock& AddError(TContext& ctx, TStringBuf code) {
    NSc::TValue json;
    json["code"].SetString(code);
    return *ctx.AddErrorBlock(TError(TError::EType::TIMERERROR), std::move(json));
}

void ShowTimersAction(TContext& ctx) {
    if (!ctx.ClientFeatures().SupportsTimersShowResponse()) {
        return;
    }

    if (ctx.ClientFeatures().IsTouch()) {
        ctx.AddAttention(ATTBLOCK_IS_MOBILE);
    }
    ctx.AddCommand<TTimerShowDirective>(ACTION_SHOW_TIMERS, NSc::Null());
}

void CancelTimerAction(TContext& ctx, TStringBuf id) {
    NSc::TValue payload;
    payload["timer_id"].SetString(id);
    ctx.AddCommand<TTimerCancelDirective>(TStringBuf("cancel_timer"), std::move(payload), true /* beforeTts */);
}

void PauseTimerAction(TContext& ctx, TStringBuf id) {
    NSc::TValue payload;
    payload["timer_id"].SetString(id);
    ctx.AddCommand<TTimerPauseDirective>(TStringBuf("pause_timer"), std::move(payload), true /* beforeTts */);
}

void ResumeTimerAction(TContext& ctx, TStringBuf id) {
    NSc::TValue payload;
    payload["timer_id"].SetString(id);
    ctx.AddCommand<TTimerResumeDirective>(TStringBuf("resume_timer"), std::move(payload), true /* beforeTts */);
}

void ResponseAsUnsupported(TContext& ctx) {
    ctx.AddOnboardingSuggest();
    AddError(ctx, ERROR_CODE_UNSUPPORTED);
}

void AddPredefinedCommonSuggests(TContext& ctx) {
    if (ctx.ClientFeatures().SupportsTimersShowResponse()) {
        NSc::TValue command, data;
        command["command_type"].SetString(ACTION_SHOW_TIMERS);
        command["data"].SetNull();
        data["commands"].SetArray().Push(command);
        ctx.AddSuggest(TStringBuf("timer__show_timers"), std::move(data));
    }

    if (ctx.ClientFeatures().SupportsTimers() && !ctx.MetaClientInfo().IsSmartSpeaker()) {
        for (const i64 minutes : SUGGESTED_MINUTES) {
            NSc::TValue payload;
            payload["time"]["minutes"].SetIntNumber(minutes);
            ctx.AddSuggest(TStringBuf("timer__set_timer"), std::move(payload));
        }
    }
}

void ResponseAsShowTimers(TContext& ctx) {
    AddPredefinedCommonSuggests(ctx);
    ShowTimersAction(ctx);
}

void TryResponseAsShowTimers(TContext& ctx) {
    if (ctx.ClientFeatures().SupportsTimersShowResponse()) {
        ResponseAsShowTimers(ctx);
    } else {
        ResponseAsUnsupported(ctx);
    }
}

bool IsSleepTimer(const TTimer::TScheme& timerScheme) {
    return timerScheme.HasDirectives() &&
           NImpl::AreJsonDirectivesEqualByName(*timerScheme.Directives().GetRawValue(), SLEEP_DIRECTIVES);
}

TMaybe<TString> TryGetSleepTimerId(const TContext& ctx) {
    for (const auto& timer : ctx.Meta().DeviceState().Timers().ActiveTimers()) {
        if (IsSleepTimer(timer)) {
            return TString(timer.Id());
        }
    }
    return Nothing();
}

TMaybe<NDatetime::TTimeZone> TryGetTimeZone(TContext& ctx) {
    try {
        return NDatetime::GetTimeZone(ctx.UserTimeZone());
    } catch (const NDatetime::TInvalidTimezone& e) {
        LOG(ERR) << "Invalid time zone: " << ctx.UserTimeZone() << ' ' << e.what() << Endl;
        ctx.AddErrorBlock(TError{TError::EType::SYSTEM}, "time_zone");
        return Nothing();
    }
}

NSc::TValue TryConvertToTimeValue(const ui64 timestamp, TContext& ctx) {
    if (const auto tz = TryGetTimeZone(ctx)) {
        return NAlice::NScenarios::NAlarm::TimeToValue(NDatetime::Convert(TInstant::Seconds(timestamp), *tz));
    }
    return NSc::Null();
}

TTimer::TTimer(TDuration duration, ETimerType timerType)
    : Duration(duration)
    , Type(timerType)
{
    const TDuration::TValue seconds = Duration.Seconds();
    Data.reserve(Sources.size());
    for (const STimeSource& source : Sources) {
        Data.emplace_back(source, source.Init(seconds));
    }
}

TTimer::TTimer(TInstant timestamp, const NSc::TValue& timeValue, ETimerType timerType)
    : Timestamp(timestamp)
    , TimeValue(timeValue)
    , Type(timerType)
{
}

void TTimer::ToJson(NSc::TValue* json) const {
    for (const auto& kv : Data) {
        (*json)[kv.first.UnitName] = kv.second;
    }
}

void TTimer::UpdateForm(TContext& ctx) const {
    if (TimeValue) {
        CreateTimeSlot(ctx)->Value = *TimeValue;
    } else {
        ToJson(&CreateTimeUnitsSlot(ctx)->Value);
    }
    UpdateSpecificationSlot(ctx, Type);
}

// static
TSlot* TTimer::CreateTimeSlot(TContext& ctx) {
    ctx.AddAttention(ATTBLOCK_ABS_TIME);
    return ctx.CreateSlot(SLOT_NAME_TIME, SLOT_TYPE_TIME, true);
}

// static
TSlot* TTimer::CreateTimeUnitsSlot(TContext& ctx) {
    return ctx.CreateSlot(SLOT_NAME_TIME, SLOT_TYPE_TIMEUNITS, true);
}

// static
void TTimer::UpdateForm(const NSc::TValue& timer, TContext& ctx) {
    const auto& duration = timer.TrySelect("duration");
    if (!duration.IsNull()) {
        CreateTimeUnitsSlot(ctx)->Value = duration;
        return;
    }

    const auto& timeValue = timer.TrySelect("time");
    if (!timeValue.IsNull()) {
        CreateTimeSlot(ctx)->Value = timeValue;
    }

    ETimerType timerType = timer.TrySelect("specification").GetString() == ToString(ETimerType::Sleep) ?
        ETimerType::Sleep : ETimerType::Normal;
    UpdateSpecificationSlot(ctx, timerType);
}

// static
void TTimer::UpdateForm(TTimer::TScheme timerScheme, TContext& ctx) {
    TMaybe<TTimer> timer;
    ETimerType timerType = IsSleepTimer(timerScheme) ? ETimerType::Sleep : ETimerType::Normal;
    if (timerScheme.HasDuration()) {
        timer.ConstructInPlace(TDuration::Seconds(timerScheme.Duration()), timerType);
    } else if (timerScheme.HasTimestamp()) {
        auto timeValue = TryConvertToTimeValue(timerScheme.Timestamp(), ctx);
        if (!timeValue.IsNull()) {
            timer.ConstructInPlace(TInstant::Seconds(timerScheme.Timestamp()), timeValue, timerType);
        }
    }

    if (timer) {
        timer->UpdateForm(ctx);
        return;
    }
    LOG(ERR) << "No data to constuct timer from TScheme" << Endl;
}

// static
void TTimer::ToJson(TTimer::TScheme timerScheme, TContext& ctx, NSc::TValue* json) {
    (*json)["timer_id"].SetString(timerScheme.Id());
    (*json)["paused"].SetBool(timerScheme.IsPaused());
    TTimer{TDuration::Seconds(timerScheme.Remaining())}.ToJson(&(*json)["remaining"]);
    if (timerScheme.HasDuration()) {
        TTimer{TDuration::Seconds(timerScheme.Duration())}.ToJson(&(*json)["duration"]);
    }
    if (timerScheme.HasTimestamp()) {
        (*json)["time"] = TryConvertToTimeValue(timerScheme.Timestamp(), ctx);
    }
    if (IsSleepTimer(timerScheme)) {
        (*json)["specification"].SetString(ToString(ETimerType::Sleep));
    }
}

// static
TTimer::TList TTimer::GetDeviceTimers(const TContext& ctx) {
    static constexpr ui64 maxSecondsAllowed = MAX_DURATION_ALLOWED.Seconds();

    TList timers;
    const auto& srcTimers = ctx.Meta().DeviceState().Timers().ActiveTimers();
    const bool isSleepTimerRequest = IsSleepTimerRequest(ctx);
    for (const auto& t : srcTimers) {
        if (isSleepTimerRequest && !IsSleepTimer(t)) {
            continue;
        }
        timers.emplace_back(t);
    }

    Sort(timers.begin(), timers.end(), [](const auto& l, const auto& r) {
         return l.IsPaused() * maxSecondsAllowed + l.Remaining() < r.IsPaused() * maxSecondsAllowed + r.Remaining();
         }
    );

    return timers;
}

void TTimer::CopySlots(const TContext& from, TContext* to) {
    to->CopySlotFrom(from, SLOT_NAME_TIME);
}

void CreateAvailableTimersSlot(const TTimer::TList& timers, TContext& ctx) {
    NSc::TValue timersForSlot;

    for (const auto& t : timers) {
        TTimer::ToJson(t, ctx, &timersForSlot.Push());
    }

    ctx.CreateSlot(SLOT_NAME_AVAILTIMERS, SLOT_TYPE_AVAILTIMERS, true, std::move(timersForSlot));
}

bool TTimer::FromSlots(TContext& ctx, TTimerVal* timer) {
    Y_ASSERT(timer);

    return FromTimeUnitsSlot(ctx, timer) && FromTimeSlot(ctx, timer);
}

bool TTimer::FromTimeUnitsSlot(TContext& ctx, TTimerVal* timer) {
    const TSlot* const slot = ctx.GetSlot(SLOT_NAME_TIME, SLOT_TYPE_TIMEUNITS);
    if (IsSlotEmpty(slot)) {
        return true;
    }

    TMaybe<TDuration> duration;
    for (const auto& s : Sources) {
        const double value = slot->Value[s.UnitName].GetNumber(-1);
        if (value > 0) {
            if (!duration) {
                duration.ConstructInPlace();
            }
            *duration += s.Multiplier * value;
        }
    }

    if (!duration) {
        return true;
    }

    timer->ConstructInPlace(*duration, InferTimerTypeFromCtx(ctx));
    if (!(*timer)->Duration.Seconds() || (*timer)->Duration >= MAX_DURATION_ALLOWED) {
        AddError(ctx, ERROR_CODE_DURATION_TOO_LONG);
        return false;
    }

    return true;
}

bool TTimer::FromTimeSlot(TContext& ctx, TTimerVal* timer) {
    const TSlot* const slot = ctx.GetSlot(SLOT_NAME_TIME, SLOT_TYPE_TIME);
    if (IsSlotEmpty(slot)) {
        return true;
    }

    const TMaybe<NAlice::NScenarios::NAlarm::TDayTime> timeFromSlot{NAlice::NScenarios::NAlarm::TDayTime::FromValue(slot->Value)};
    if (!timeFromSlot) {
        LOG(ERR) << "Error creating TDayTime from time slot" << Endl;
    }
    if (!timeFromSlot || timeFromSlot->HasRelativeNegative()) {
        AddError(ctx, ERROR_CODE_INVALID_TIME);
        return false;
    }

    auto tz = TryGetTimeZone(ctx);
    if (!tz) {
        return false;
    }

    TInstant currentTimestamp = GetCurrentTimestamp(ctx);
    NDatetime::TCivilSecond currentTime = NDatetime::Convert(currentTimestamp, *tz);
    NDatetime::TCivilSecond shootTime = GetAlarmTime(currentTime, *timeFromSlot);
    TInstant shootTimestamp = NDatetime::Convert(shootTime, *tz);
    ETimerType timerType = InferTimerTypeFromCtx(ctx);

    if (timeFromSlot->IsRelative()) {
        timer->ConstructInPlace(TDuration::FromValue(shootTimestamp.MicroSeconds() - currentTimestamp.MicroSeconds()), timerType);
    } else {
        timer->ConstructInPlace(shootTimestamp, NAlice::NScenarios::NAlarm::TimeToValue(shootTime), timerType);
    }

    return true;
}

bool IsEnabled(TContext& ctx) {
    if (ctx.ClientFeatures().SupportsTimers()) {
        if (IsSleepTimerRequest(ctx) && !ctx.HasExpFlag(EXPERIMENTAL_FLAG_SLEEP_TIMERS)) {
            ctx.CreateSlot(
                SLOT_NAME_SPECIFICATION,
                SLOT_TYPE_SPECIFICATION,
                true /* optional */,
                NSc::Null() /* value */
            );
        }
        return true;
    }

    ResponseAsUnsupported(ctx);
    return false;
}

TTimerVal ObtainTimerWithDefaultAction(TContext& ctx, std::function<void(TContext&, TStringBuf)> action) {
    if (!IsEnabled(ctx)) {
        return Nothing();
    }

    if (!ctx.MetaClientInfo().IsSmartSpeaker()) {
        TryResponseAsShowTimers(ctx);
        return Nothing();
    }

    const TSlot* const slotTimerId = ctx.GetSlot(SLOT_NAME_TIMERID);
    if (IsSlotEmpty(slotTimerId)) {
        TTimerVal timer;
        if (!TTimer::FromSlots(ctx, &timer)) {
            return Nothing();
        }

        if (timer) {
            timer->UpdateForm(ctx);
            return timer;
        }

        const auto timers = TTimer::GetDeviceTimers(ctx);
        if (!timers.size()) {
            AddError(ctx, ERROR_CODE_NOTIMERS);
        } else if (timers.size() != 1) {
            CreateAvailableTimersSlot(timers, ctx);
            // XXX not sure that this attention block is really needed
            ctx.AddAttention("timer__no_timer_specified");
        } else {
            const TTimer::TScheme timerScheme{timers[0]};
            TTimer::UpdateForm(timerScheme, ctx);
            action(ctx, timers[0].Id());
        }

        return Nothing();
    }

    if (slotTimerId->Type == TStringBuf("selection")) {
        for (const auto& timer : TTimer::GetDeviceTimers(ctx)) {
            action(ctx, timer.Id());
        }

        return Nothing();
    }

    TSlot* const availableTimersSlot = ctx.GetSlot(SLOT_NAME_AVAILTIMERS);
    if (IsSlotEmpty(availableTimersSlot)) {
        CreateAvailableTimersSlot(TTimer::GetDeviceTimers(ctx), ctx);
        ctx.AddAttention(ATTBLOCK_MULTIPLE_TIMERS);
        return Nothing();
    }

    i64 idx = slotTimerId->Value.GetIntNumber(-1);
    const NSc::TArray& availableTimers = availableTimersSlot->Value.GetArray();
    if (!availableTimers) {
        AddError(ctx, ERROR_CODE_BADARGS);
        return Nothing();
    }
    if (idx < 0 || static_cast<ui64>(idx) > availableTimers.size()) {
        AddError(ctx, ERROR_CODE_BADARGS);
        return Nothing();
    }

    const NSc::TValue& timer = availableTimers[idx == 0 ? idx : idx - 1];
    TTimer::UpdateForm(timer, ctx);
    action(ctx, timer["timer_id"].GetString());

    availableTimersSlot->Value.SetNull();

    return Nothing();
}

void SetTimerProductScenario(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::TIMER);
}

void SetSleepTimerProductScenario(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SLEEP_TIMER);
}

} // namespace

namespace NImpl {

bool AreJsonDirectivesEqualByName(const NSc::TValue& lhs, const NSc::TValue& rhs) {
    if (!lhs.IsArray() || !rhs.IsArray()) {
        return false;
    }
    const auto& lhsArray = lhs.GetArray();
    const auto& rhsArray = rhs.GetArray();
    if (lhsArray.size() != rhsArray.size()) {
        return false;
    }
    constexpr auto hasInvalidStructure = [](const NSc::TValue& value) {
        return !value.IsDict() || !value.GetDict().contains(DIRECTIVE_NAME_KEY);
    };
    if (AnyOf(lhsArray, hasInvalidStructure) || AnyOf(rhsArray, hasInvalidStructure)) {
        return false;
    }
    constexpr auto getName = [](const NSc::TValue& value) { return value.GetDict().at(DIRECTIVE_NAME_KEY); };
    for (const auto&[l, r] : Zip(MakeMappedRange(lhsArray, getName), MakeMappedRange(rhsArray, getName))) {
        if (l != r) {
            return false;
        }
    }
    return true;
}

} // namespace NImpl

// wiki: https://wiki.yandex-team.ru/assistant/dialogs/timer/#opisaniescenarijaotmenittajjmer
void TimerCancel(TContext& ctx) {
    SetTimerProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    const TTimerVal timer{ObtainTimerWithDefaultAction(ctx, CancelTimerAction)};
    if (!timer) {
        return;
    }

    TTimer::TList timers;
    const TTimer::TList deviceTimers{TTimer::GetDeviceTimers(ctx)};
    for (const TTimer::TScheme& t : deviceTimers) {
        if (timer->Duration.Seconds() == t.Duration()) {
            timers.emplace_back(t);
        }
    }

    if (!timers) {
        if (deviceTimers.size() == 1) {
            // requested timer is not found but has one for different time
            // ask user for confirmation via create attention block and adding
            // available_timers slot with only one timer
            ctx.AddAttention(ATTBLOCK_NOTIMERS_FOR);

            CreateAvailableTimersSlot(deviceTimers, ctx);
            // this slot is the default answer for user reply "yes"
            ctx.CreateSlot(SLOT_NAME_TIMERID, "num", true, NSc::TValue(1));
        } else {
            AddError(ctx, ERROR_CODE_NOTIMERS);
        }

        return;
    }

    // more than one timer found for the requested duration
    // ask user which one he/she wants
    if (timers.size() > 1) {
        CreateAvailableTimersSlot(timers, ctx);
        ctx.AddAttention(ATTBLOCK_MULTIPLE_TIMERS);
        return;
    }

    // only one timer found for requested duration
    CancelTimerAction(ctx, timers[0].Id());
}

void TimerShow(TContext& ctx) {
    SetTimerProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    ctx.CreateSlot(SLOT_NAME_AVAILTIMERS, SLOT_TYPE_AVAILTIMERS, true, NSc::TValue().SetArray());

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        const auto deviceTimers{TTimer::GetDeviceTimers(ctx)};
        if (!deviceTimers.size()) {
            AddError(ctx, ERROR_CODE_NOTIMERS);
            return;
        }

        ShowTimersAction(ctx);
        return CreateAvailableTimersSlot(deviceTimers, ctx);
    }

    TryResponseAsShowTimers(ctx);
}

// wiki: https://wiki.yandex-team.ru/assistant/dialogs/timer/Vins-Bass-protokol/#ustanovittajjmer
void TimerSet(TContext& ctx) {
    SetTimerProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    if (ctx.ClientFeatures().SupportsNoReliableSpeakers() && !IsSleepTimerRequest(ctx)) {
        ResponseAsUnsupported(ctx);
        return;
    }

    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        if (ctx.Meta().DeviceState().Timers().ActiveTimers().Size() >= MAX_TIMERS_ON_QUASAR) {
            AddError(ctx, ERROR_CODE_TOO_MANY_TIMERS);
            return;
        }

        TTimerVal timer;
        if (!TTimer::FromSlots(ctx, &timer)) {
            return;
        }

        if (!timer) {
            ctx.CreateSlot(SLOT_NAME_TIME, SLOT_TYPE_TIMEUNITS, false);
            return;
        }

        timer->UpdateForm(ctx);

        NSc::TValue payload;
        if (timer->Timestamp) {
            payload["timestamp"] = timer->Timestamp->Seconds();
        } else {
            payload["duration"] = timer->Duration.Seconds();
        }
        if (IsSleepTimerRequest(ctx)) {
            payload["directives"] = SLEEP_DIRECTIVES;
            auto sleepTimerId = TryGetSleepTimerId(ctx);
            if (sleepTimerId) {
                CancelTimerAction(ctx, *sleepTimerId);
            }
        } else {
            payload["listening_is_possible"].SetBool(true);
        }
        ctx.AddCommand<TTimerSetDirective>(ACTION_SET_TIMER, std::move(payload), true /* beforeTts */);

        if (!timer->Timestamp && timer->Duration.Seconds() > 3610) { // one hour + time of tts
            if (ctx.ClientFeatures().SupportsScledDisplay()) {
                const TInstant now = GetCurrentTimestamp(ctx);
                const NDatetime::TTimeZone tz = NDatetime::GetTimeZone(ctx.UserTimeZone());

                ctx.AddCommand<TDrawScledAnimationsDirective>(
                    "draw_scled_animations",
                    MakeScledTimeDirective(timer->Duration,  NDatetime::Convert(now, tz)),
                    true /* beforeTts */
                );
            }
        }

        ctx.AddStopListeningBlock();
        return;
    }

    const TSlot* const slotConfirm = ctx.GetSlot(SLOT_NAME_CONFIRMATION, SLOT_TYPE_CONFIRMATION);
    if (!IsSlotEmpty(slotConfirm)) {
        if (slotConfirm->Value.GetBool(false)) {
            AddPredefinedCommonSuggests(ctx);
        } else {
            ctx.AddOnboardingSuggest();
        }
        return;
    }

    TTimerVal timer;
    if (!TTimer::FromSlots(ctx, &timer)) {
        AddPredefinedCommonSuggests(ctx);
        return;
    }

    if (!timer) {
        ctx.CreateSlot(SLOT_NAME_TIME, SLOT_TYPE_TIMEUNITS, false);
        AddPredefinedCommonSuggests(ctx);
        return;
    }

    timer->UpdateForm(ctx);

    NSc::TValue payload;
    payload["timestamp"] = timer->Duration.Seconds();

    TContext updateForm{ctx, TIMER_FORM_NAME_SET};
    timer->CopySlots(ctx, &updateForm);
    TSlot* confirmation = updateForm.CreateSlot(SLOT_NAME_CONFIRMATION, SLOT_TYPE_CONFIRMATION, true);

    confirmation->Value.SetBool(false);
    payload["on_fail"] = updateForm.ToJson(TContext::EJsonOut::Resubmit | TContext::EJsonOut::FormUpdate).Clone();

    confirmation->Value.SetBool(true);
    payload["on_success"] = updateForm.ToJson(TContext::EJsonOut::Resubmit | TContext::EJsonOut::FormUpdate).Clone();

    ctx.AddCommand<TTimerSetDirective>(ACTION_SET_TIMER, std::move(payload));
    ctx.AddAttention("timer__need_confirmation");
}

// wiki: https://wiki.yandex-team.ru/assistant/dialogs/timer/Vins-Bass-protokol/#postavittajjmernapauzu
void TimerPause(TContext& ctx) {
    SetTimerProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    const TTimerVal timer{ObtainTimerWithDefaultAction(ctx, PauseTimerAction)};
    if (!timer) {
        return;
    }

    const TSplittedTimers timers{ctx, timer->Duration.Seconds()};

    if (!timers.Active()) {
        if (!timers.Paused()) {
            // there aren't requested timers (neither paused nor active)
            if (timers.Device.size() == 1) {
                // user has only one timer but for different time
                // confirm user to pause this one
                if (timers.Device[0].IsPaused()) {
                    ctx.AddAttention("timer__no_timers_for_time_but_paused");
                } else {
                    ctx.AddAttention(ATTBLOCK_NOTIMERS_FOR);
                }

                CreateAvailableTimersSlot(timers.Device, ctx);
            } else {
                AddError(ctx, ERROR_CODE_NOTIMERS);
            }
        } else {
            // there are requested timers (or one), but its on pause now
            AddError(ctx, "already_paused")["data"]["amount"].SetIntNumber(timers.Paused().size());
        }
    } else if (timers.Active().size() == 1) {
        PauseTimerAction(ctx, timers.Active()[0].Id());
    } else {
        // found more than one requested timer (ask user to choose one)
        CreateAvailableTimersSlot(timers.Active(), ctx);
        ctx.AddAttention(ATTBLOCK_MULTIPLE_TIMERS);
    }
}

// wiki: https://wiki.yandex-team.ru/assistant/dialogs/timer/Vins-Bass-protokol/#vozobnovittajjmerposlepauzy
// TODO have to check if it is really works (copypasted from paused form)
void TimerResume(TContext& ctx) {
    SetTimerProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    const TTimerVal timer{ObtainTimerWithDefaultAction(ctx, ResumeTimerAction)};
    if (!timer) {
        return;
    }

    const TSplittedTimers timers{ctx, timer->Duration.Seconds()};

    if (!timers.Active()) {
        if (!timers.Paused()) {
            // there aren't requested timers (neither paused nor active)
            if (timers.Device.size() == 1) {
                // user has only one timer but for different time
                // confirm user to resume this one
                if (timers.Device[0].IsPaused()) {
                    ctx.AddAttention("timer__no_timers_for_time_but_playing"); // FIXME update wiki
                } else {
                    ctx.AddAttention(ATTBLOCK_NOTIMERS_FOR);
                }

                CreateAvailableTimersSlot(timers.Device, ctx);
            } else {
                AddError(ctx, ERROR_CODE_NOTIMERS);
            }
        } else {
            // there are requested timers (or one), but its on pause now
            AddError(ctx, "already_playing")["data"]["amount"].SetIntNumber(timers.Paused().size());
        }
    } else if (timers.Active().size() == 1) {
         ResumeTimerAction(ctx, timers.Active()[0].Id());
    } else {
        // found more than one requested timer (ask user to choose one)
        CreateAvailableTimersSlot(timers.Active(), ctx);
        ctx.AddAttention(ATTBLOCK_MULTIPLE_TIMERS);
    }
}

void TimerStopPlaying(TContext& ctx) {
    SetTimerProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    if (!ctx.MetaClientInfo().IsSmartSpeaker()) {
        return TryResponseAsShowTimers(ctx);
    }

    bool isAnyTimerPlaying = false;
    for (const auto& timer : TTimer::GetDeviceTimers(ctx)) {
        if (timer.IsPlaying()) {
            isAnyTimerPlaying = true;

            NSc::TValue timerJson;
            timerJson["timer_id"].SetString(timer.Id());
            ctx.AddCommand<TTimerStopPlayingDirective>(TStringBuf("timer_stop_playing"), std::move(timerJson));
        }
    }

    if (!isAnyTimerPlaying) {
        AddError(ctx, ERROR_CODE_NOTIMERS);
    }
}

void TimerHowLong(TContext& ctx) {
    SetTimerProductScenario(ctx);
    if (!IsEnabled(ctx)) {
        return;
    }

    if (!ctx.MetaClientInfo().IsSmartSpeaker()) {
        TryResponseAsShowTimers(ctx);
    }

    // sorted by paused and remaining time
    const auto timers{TTimer::GetDeviceTimers(ctx)};
    if (!timers.size()) {
        AddError(ctx, ERROR_CODE_NOTIMERS);
        return;
    }

    const auto& timer = timers[0];
    if (timer.IsPaused()) {
        ctx.AddAttention(ATTBLOCK_IS_PAUSED);
    }
    if (IsSleepTimer(timer)) {
        UpdateSpecificationSlot(ctx, ETimerType::Sleep);
    }

    CreateHowLongSlot(ctx, NAlice::NScenarios::NAlarm::TDayTime(0, 0, timer.Remaining(), NAlice::NScenarios::NAlarm::TDayTime::EPeriod::Unspecified));
}

void SleepTimerSet(TContext& ctx) {
    Y_DEFER {
        SetSleepTimerProductScenario(ctx);
    };
    UpdateSpecificationSlot(ctx, ETimerType::Sleep);
    const TSlot* const now = ctx.GetSlot(SLOT_NAME_NOW, SLOT_TYPE_NOW);
    if (!IsSlotEmpty(now)) {
        for(const NSc::TValue& data : SLEEP_DIRECTIVES.GetArray()) {
            ctx.AddCommand<TTimerSleepNowDirective>(data.Get("name"), data.Get("payload"));
        }
        ctx.AddSilentResponse();
        return;
    }
    TimerSet(ctx);
}

void SleepTimerHowLong(TContext& ctx) {
    Y_DEFER {
        SetSleepTimerProductScenario(ctx);
    };
    UpdateSpecificationSlot(ctx, ETimerType::Sleep);
    TimerHowLong(ctx);
}

} // namespace NReminders
} // namespace NBASS
