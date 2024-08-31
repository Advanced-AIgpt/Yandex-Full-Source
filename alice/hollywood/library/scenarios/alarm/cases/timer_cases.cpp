#include "timer_cases.h"

#include <alice/library/scenarios/alarm/helpers.h>

#include <alice/library/url_builder/url_builder.h>
#include <alice/library/proto/protobuf.h>

namespace NAlice {

constexpr size_t MAX_NUM_TIMERS = 25;
constexpr TDuration MAX_DURATION_ALLOWED = TDuration::Days(1);

constexpr i64 SUGGESTED_MINUTES[] = { 5, 10, 15 };


struct STimeSource {
    const TStringBuf UnitName;
    const TDuration Multiplier;
    const std::function<ui64(ui64)> Init;
};

const TVector<STimeSource> Sources = {
    { "hours", TDuration::Hours(1),   [](ui64 duration) { return duration / 3600; } },
    { "minutes", TDuration::Minutes(1), [](ui64 duration) { return (duration % 3600) / 60; } },
    { "seconds", TDuration::Seconds(1), [](ui64 duration) { return (duration % 3600) % 60; } },
};

struct TTimer {
    TTimer(TDuration duration, bool isSleepTimer)
        : Duration(duration)
        , IsSleepTimer(isSleepTimer)
    {
    }

    TTimer(TInstant timestamp, bool isSleepTimer)
        : Timestamp(timestamp)
        , IsSleepTimer(isSleepTimer)
    {
    }

    static TMaybe<TTimer> FromTimeSlot(
        NAlice::NHollywood::NReminders::TRemindersContext& ctx,
        const NAlice::NHollywood::TPtrWrapper<NAlice::NHollywood::TSlot>& timeSlot,
        bool isSleepTimer
    ) {
        if (NHollywood::NReminders::IsSlotEmpty(timeSlot)) {
            return Nothing();
        }

        NJson::TJsonValue time;
        NJson::ReadJsonFastTree(timeSlot->Value.AsString(), &time, false);

        if (timeSlot->Type == NAlice::NHollywood::NReminders::NSlots::TYPE_UNITSTIME) {
            TMaybe<TDuration> duration;
            for (const auto& s : Sources) {
                const double value = time[s.UnitName].GetDouble();
                if (value > 0) {
                    if (!duration) {
                        duration.ConstructInPlace();
                    }
                    *duration += s.Multiplier * value;
                }
            }

            if (duration && duration->Seconds() && duration < MAX_DURATION_ALLOWED) {
                return TTimer(*duration, isSleepTimer);
            }

        } else if (timeSlot->Type == NAlice::NHollywood::NReminders::NSlots::TYPE_TYPEPARSERTIME) {
            auto dayTime = NAlice::NScenarios::NAlarm::TDayTime::FromValue(NSc::TValue::FromJsonValue(time));

            const TInstant currentTimestamp = GetCurrentTimestamp(ctx);
            NDatetime::TTimeZone tz;
            try {
                tz = NDatetime::GetTimeZone(ctx.GetTimezone());
            } catch (const NDatetime::TInvalidTimezone& e) {
                return Nothing();
            }

            if (dayTime && !dayTime->HasRelativeNegative()) {
                NDatetime::TCivilSecond currentTime = NDatetime::Convert(currentTimestamp, tz);
                NDatetime::TCivilSecond shootTime = GetAlarmTime(currentTime, *dayTime);
                TInstant shootTimestamp = NDatetime::Convert(shootTime, tz);

                if (dayTime->IsRelative()) {
                    return TTimer(shootTimestamp - currentTimestamp, isSleepTimer);
                }
                return TTimer(shootTimestamp,  isSleepTimer);
            }
        }

        return Nothing();
    }

    const TMaybe<TDuration> Duration;
    const TMaybe<TInstant> Timestamp;
    bool IsSleepTimer;
};

NJson::TJsonValue GetTimeByDuration(const TDuration& duration) {
    NJson::TJsonValue time;
    for (auto s : Sources) {
        time[s.UnitName] = s.Init(duration.Seconds());
    }
    return time;
}

NJson::TJsonValue GetTime(
    const NAlice::TDeviceState::TTimers::TTimer& timer
) {
    return GetTimeByDuration(TDuration::Seconds(timer.GetDuration()));
}

TMaybe<NAlice::TDeviceState::TTimers::TTimer> GetTimerById(
    const TVector<NAlice::TDeviceState::TTimers::TTimer>& timers,
    const TStringBuf& timerId
) {
    for (const auto& timer : timers) {
        if (timer.GetTimerId() == timerId) {
            return timer;
        }
    }
    return Nothing();
}

bool ClientSupportsTimers(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    return ctx.GetInterfaces().GetCanSetTimer();
}

void SetTimerProductScenario(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    ctx.Renderer().SetProductScenarioName(NAlice::NHollywood::NReminders::NScenarioNames::TIMER.Data());
}

void SetSleepTimerProductScenario(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    ctx.Renderer().SetProductScenarioName(NAlice::NHollywood::NReminders::NScenarioNames::SLEEP_TIMER.Data());
}

void SetTimerAnalyticsInfo(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
) {
    if (isSleepTimerRequest) {
        SetSleepTimerProductScenario(ctx);
    } else {
        SetTimerProductScenario(ctx);
    }

    ctx.Renderer().SetIntentName(frameName.Data());
}

bool IsEnabled(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TStringBuf& nlgTemplate
) {
    if (ClientSupportsTimers(ctx)) {
        return true;
    }

    if (ctx.RunRequest().ClientInfo().IsNavigator()) {
        ctx.Renderer().SetIrrelevant();
    }

    RaiseErrorShowPromo(
        ctx,
        nlgTemplate,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
    );

    return false;
}

void CancelTimerAction(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TStringBuf& timerId
) {
    NAlice::NScenarios::TDirective directive;
    auto* cancelTimerDirective = directive.MutableCancelTimerDirective();
    cancelTimerDirective->SetName("cancel_timer");
    cancelTimerDirective->SetTimerId(TString(timerId));

    ctx.Renderer().AddDirective(std::move(directive));
}

void PauseTimerAction(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TStringBuf& timerId
) {
    NAlice::NScenarios::TDirective directive;
    auto* pauseTimerDirective = directive.MutablePauseTimerDirective();
    pauseTimerDirective->SetName("pause_timer");
    pauseTimerDirective->SetTimerId(TString(timerId));

    ctx.Renderer().AddDirective(std::move(directive));
}

void ResumeTimerAction(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TStringBuf& timerId
) {
    NAlice::NScenarios::TDirective directive;
    auto* resumeTimerDirective = directive.MutableResumeTimerDirective();
    resumeTimerDirective->SetName("resume_timer");
    resumeTimerDirective->SetTimerId(TString(timerId));

    ctx.Renderer().AddDirective(std::move(directive));
}

void AddSleepDirectives(NAlice::NScenarios::TSetTimerDirective* setTimerDirective) {
    {
        auto* directive = setTimerDirective->AddDirectives();
        auto* playerPauseDirective = directive->MutablePlayerPauseDirective();
        playerPauseDirective->SetName("player_pause");
        playerPauseDirective->SetSmooth(true);
    }

    {
        auto* directive = setTimerDirective->AddDirectives();
        auto* clearQueueDirective = directive->MutableClearQueueDirective();
        clearQueueDirective->SetName("clear_queue");
    }

    {
        auto* directive = setTimerDirective->AddDirectives();
        auto* goHomeDirective = directive->MutableGoHomeDirective();
        goHomeDirective->SetName("go_home");
    }

    {
        auto* directive = setTimerDirective->AddDirectives();
        auto* screenOffDirective = directive->MutableScreenOffDirective();
        screenOffDirective->SetName("screen_off");
    }
}

TMaybe<TString> TryGetSleepTimerId(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    for (const auto& timer : ctx.GetTimers().GetActiveTimers()) {
        if (timer.GetDirectives().size()) {
            return TString(timer.GetTimerId());
        }
    }
    return Nothing();
}

void AddTimerSetSuggests(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    for (const auto& minutes : SUGGESTED_MINUTES) {
        NJson::TJsonValue suggestData;
        suggestData["time"]["minutes"] = minutes;
        ctx.Renderer().AddTypeTextSuggest(
            "set_timer",
            suggestData,
            TString(TStringBuilder{} << "suggest_set_timer_on_" << minutes)
        );
    }
}

void AddTimerShowSuggest(NAlice::NHollywood::NReminders::TRemindersContext& ctx) {
    NScenarios::TDirective directive;
    NScenarios::TShowTimersDirective* showTimersDirective = directive.MutableShowTimersDirective();
    showTimersDirective->SetName("show_timers");

    ctx.Renderer().AddDirectiveSuggest(
        "show_timers",
        NJson::TJsonValue(),
        "suggest_show_timers",
        directive,
        true /* addButton */
    );
}

bool TryShowTimersForMobile(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TStringBuf& nlgTemplate
) {
    if (!ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
        if (ctx.GetInterfaces().GetCanShowTimer()) {
            AddTimerSetSuggests(ctx);
            AddTimerShowSuggest(ctx);
            NAlice::NScenarios::TDirective directive;
            auto* showTimersDirective = directive.MutableShowTimersDirective();
            showTimersDirective->SetName("show_timers");
            ctx.Renderer().AddDirective(std::move(directive));

            ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_TIMER_IS_MOBILE);

            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT
            );

            ConstructResponse(ctx);
        } else {
            RaiseErrorShowPromo(
                ctx,
                nlgTemplate,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::NOT_SUPPORTED
            );
        }
        return true;
    }

    return false;
}

::google::protobuf::Struct MakeTimerSetPayload(bool success, const NAlice::NHollywood::TFrame& frame) {
    auto bufFrame = frame;
    auto successSlot = NAlice::NHollywood::NReminders::FindOrAddSlot(
        bufFrame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_TIMER_SET_SUCCESS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_BOOL
    );

    const_cast<NAlice::NHollywood::TSlot*>(successSlot.Get())->Value = NAlice::NHollywood::TSlot::TValue(
        NAlice::JsonToString(NJson::TJsonValue(success))
    );

    return NAlice::NHollywood::NReminders::ToPayloadUpdateForm(bufFrame.ToProto());
}

void TimerSetConfirmation(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    const TMaybe<NAlice::NHollywood::TFrame>& frame,
    const NAlice::NHollywood::TPtrWrapper<NAlice::NHollywood::TSlot>& successSlot,
    const TStringBuf nlgTemplate
) {
    ctx.Renderer().AddFrameSlots(*frame);

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

    AddTimerShowSuggest(ctx);
    return ConstructResponse(ctx);
}

void ShowMultipleTimers(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    const TStringBuf& nlgTemplate,
    const TVector<NAlice::TDeviceState::TTimers::TTimer>& timers
) {
    NJson::TJsonValue timersJson;
    timersJson["paused"] = NJson::TJsonArray();
    timersJson["not_paused"] = NJson::TJsonArray();

    for (const auto& timer : timers) {
        NJson::TJsonValue timerJson;
        timerJson["duration"] = GetTimeByDuration(TDuration::Seconds(timer.GetDuration()));
        timerJson["remaining"] = GetTimeByDuration(TDuration::Seconds(timer.GetRemaining()));
        timerJson["specification"] = timer.GetDirectives().empty() ? "normal" : "sleep";
        timerJson["timer_id"] = timer.GetTimerId();

        if (timer.GetPaused()) {
            timersJson["paused"].AppendValue(timerJson);
        } else {
            timersJson["not_paused"].AppendValue(timerJson);
        }
    }

    NJson::TJsonValue orderedTimerIds;
    for (const auto& timer : timersJson["not_paused"].GetArray()) {
        orderedTimerIds.AppendValue(timer["timer_id"]);
    }

    for (const auto& timer : timersJson["paused"].GetArray()) {
        orderedTimerIds.AppendValue(timer["timer_id"]);
    }

    auto availableTimersSlot = NAlice::NHollywood::NReminders::FindOrAddSlot(
        frame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_TIMERS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_LIST
    );
    const_cast<NAlice::NHollywood::TSlot*>(availableTimersSlot.Get())->Value = NAlice::NHollywood::TSlot::TValue(
        NAlice::JsonToString(orderedTimerIds)
    );
    ctx.State().MutableSemanticFrame()->MergeFrom(frame.ToProto());

    NJson::TJsonValue cardData;
    cardData["available_timers"] = timersJson;

    ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_MULTIPLE_TIMERS);
    ctx.Renderer().AddVoiceCard(
        nlgTemplate,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    ctx.Renderer().SetShouldListen(true);

    return ConstructResponse(ctx);
}

void ConstructResponseForTimerStateModification(
    NAlice::NHollywood::NReminders::TRemindersContext& ctx,
    NAlice::NHollywood::TFrame& frame,
    bool isSleepTimerRequest,
    std::function<void(NAlice::NHollywood::NReminders::TRemindersContext&, TStringBuf)> action,
    std::function<bool(const NAlice::TDeviceState::TTimers::TTimer&)> canAction,
    const TStringBuf nlgTemplate
) {
    const auto deviceTimers = GetDeviceTimers(ctx, isSleepTimerRequest);

    NDatetime::TTimeZone tz;
    try {
        tz = NDatetime::GetTimeZone(ctx.GetTimezone());
    } catch (const NDatetime::TInvalidTimezone& e) {
        return RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::INVALID_TIME_ZONE
        );
    }

    auto timerIdSlot = frame.FindSlot(
        NAlice::NHollywood::NReminders::NSlots::SLOT_TIMER_ID
    );

    if (NAlice::NHollywood::NReminders::IsSlotEmpty(timerIdSlot)) {
        auto timeSlot = frame.FindSlot(
            NAlice::NHollywood::NReminders::NSlots::SLOT_TIME
        );

        auto timer = TTimer::FromTimeSlot(ctx, timeSlot, isSleepTimerRequest);

        TVector<NAlice::TDeviceState::TTimers::TTimer> ableToActionTimers;
        TVector<NAlice::TDeviceState::TTimers::TTimer> notAbleToActionTimers;

        for (const auto& t : deviceTimers) {
            if (!timer.Defined() || timer->Duration->Seconds() == t.GetDuration()) {
                if (canAction(t)) {
                    ableToActionTimers.emplace_back(t);
                } else {
                    notAbleToActionTimers.emplace_back(t);
                }
            }
        }

        if (ableToActionTimers.empty()) {
            if (notAbleToActionTimers.empty()) {
                if (deviceTimers.size() == 1 && canAction(deviceTimers[0])) {
                    auto timerIdSlot = NAlice::NHollywood::NReminders::FindOrAddSlot(
                        frame,
                        NAlice::NHollywood::NReminders::NSlots::SLOT_TIMER_ID,
                        NAlice::NHollywood::NReminders::NSlots::TYPE_NUM
                    );

                    const_cast<NAlice::NHollywood::TSlot*>(timerIdSlot.Get())->Value
                        = NAlice::NHollywood::TSlot::TValue(NAlice::JsonToString(NJson::TJsonValue(1)));

                    return ShowMultipleTimers(
                        ctx,
                        frame,
                        nlgTemplate,
                        deviceTimers
                    );
                } else {
                    NJson::TJsonValue cardData;

                    if (!NHollywood::NReminders::IsSlotEmpty(timeSlot)) {
                        cardData["time"] = NAlice::JsonFromString(timeSlot->Value.AsString());
                    }

                    ctx.Renderer().AddError(NAlice::NHollywood::NReminders::NNlgTemplateNames::NO_TIMERS);
                    ctx.Renderer().AddVoiceCard(
                        nlgTemplate,
                        NAlice::NHollywood::NReminders::NNlgTemplateNames::ERROR,
                        cardData
                    );

                    return ConstructResponse(ctx);
                }
            }

            return RaiseErrorShowPromo(
                ctx,
                nlgTemplate,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::ALREADY_ACTIONED
            );
        } else if (ableToActionTimers.size() == 1) {
            action(ctx, ableToActionTimers[0].GetTimerId());
            ctx.Renderer().AddTtsPlayPlaceholderDirective();

            NJson::TJsonValue cardData;
            cardData["time"] = GetTime(ableToActionTimers[0]);
            cardData["remaining"] = GetTimeByDuration(TDuration::Seconds(ableToActionTimers[0].GetRemaining()));
            if (isSleepTimerRequest) {
                cardData["specification"] = "sleep";
            }

            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
                cardData
            );

            return ConstructResponse(ctx);
        } else {
            return ShowMultipleTimers(
                ctx,
                frame,
                nlgTemplate,
                ableToActionTimers
            );
        }
    }

    if (timerIdSlot->Type == NAlice::NHollywood::NReminders::NSlots::TYPE_SELECTION ||
        timerIdSlot->Type == NAlice::NHollywood::NReminders::NSlots::TYPE_SELECTION_DEPRECATED) {
        if (!deviceTimers.empty()) {
            for (const auto& timer : deviceTimers) {
                action(ctx, timer.GetTimerId());
            }
            ctx.Renderer().AddTtsPlayPlaceholderDirective();

            NJson::TJsonValue cardData;
            cardData["timer_id"] = isSleepTimerRequest ? "sleep_timer" : "all";

            ctx.Renderer().AddVoiceCard(
                nlgTemplate,
                NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
                cardData
            );

            return ConstructResponse(ctx);
        }

        ctx.Renderer().AddError(NAlice::NHollywood::NReminders::NNlgTemplateNames::NO_TIMERS);
        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::ERROR
        );

        return ConstructResponse(ctx);
    }

    auto availableTimersSlot = NAlice::NHollywood::NReminders::FindSlot(
        frame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_AVAILABLE_TIMERS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_LIST
    );

    if (NAlice::NHollywood::NReminders::IsSlotEmpty(availableTimersSlot)) {
        return ShowMultipleTimers(
            ctx,
            frame,
            nlgTemplate,
            deviceTimers
        );
    }

    const auto indicies = NAlice::JsonFromString(availableTimersSlot->Value.AsString()).GetArray();

    NJson::TJsonValue timerIdJson = NAlice::JsonFromString(timerIdSlot->Value.AsString());
    i64 idx = timerIdJson.IsInteger() ? timerIdJson.GetInteger() : -1;
    if (idx) {
        --idx;
    }

    if (idx < 0 || static_cast<ui64>(idx) >= indicies.size()) {
        return RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NAlice::NHollywood::NReminders::NNlgTemplateNames::BAD_ARGUMENTS
        );
    }

    auto timerId = indicies[idx].GetString();
    action(ctx, timerId);
    ctx.Renderer().AddTtsPlayPlaceholderDirective();

    NJson::TJsonValue cardData;
    if (auto timer = GetTimerById(deviceTimers, timerId); timer) {
        cardData["time"] = GetTime(*timer);
        cardData["remaining"] = GetTimeByDuration(TDuration::Seconds(timer->GetRemaining()));
    }

    ctx.Renderer().AddVoiceCard(
        nlgTemplate,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    return ConstructResponse(ctx);
}

} // namespace

namespace NAlice::NHollywood::NReminders {

void TimerSet(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    const bool isSleepTimerRequest
) {
    SetTimerAnalyticsInfo(
        ctx,
        frameName,
        isSleepTimerRequest
    );

    if (!IsEnabled(ctx, nlgTemplate)) {
        return;
    }

    if (!ctx.RunRequest().Interfaces().GetHasReliableSpeakers()) {
        return RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NNlgTemplateNames::NOT_SUPPORTED
        );
    }

    if (const auto& frame = TryGetCallbackUpdateFormFrame(ctx.RunRequest().Input().GetCallback()); frame) {
        const auto successSlot = frame->FindSlot(NSlots::SLOT_TIMER_SET_SUCCESS);
        if (!IsSlotEmpty(successSlot)) {
            return TimerSetConfirmation(
                ctx,
                frame,
                successSlot,
                nlgTemplate
            );
        }
    }

    if (static_cast<size_t>(ctx.GetTimers().GetActiveTimers().size()) >= MAX_NUM_TIMERS) {
        return RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NNlgTemplateNames::TOO_MANY_TIMERS
        );
    }

    auto frame = ctx.RunRequest().Input().CreateRequestFrame(frameName);
    FillSlotsFromState(frame, ctx.State());
    ctx.Renderer().AddFrameSlots(frame);

    const auto timeSlot = frame.FindSlot(NSlots::SLOT_TIME) ? FindSingleSlot(frame, NSlots::SLOT_TIME) : FindSingleSlot(frame, NSlots::SLOT_TYPEPARSER_TIME);

    if (IsSlotEmpty(timeSlot)) {
         ctx.State().MutableSemanticFrame()->MergeFrom(frame.ToProto());

        ctx.Renderer().AddVoiceCard(
            nlgTemplate,
            NNlgTemplateNames::ASK_TIME
        );
        ctx.Renderer().SetShouldListen(true);

        if (!ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
            AddTimerSetSuggests(ctx);
            AddTimerShowSuggest(ctx);
        }

        return ConstructResponse(ctx);
    }

    NJson::TJsonValue time;
    NJson::ReadJsonFastTree(timeSlot->Value.AsString(), &time, false);

    NScenarios::TDirective directive;
    auto* timerSetDirective = directive.MutableSetTimerDirective();

    const auto timer = TTimer::FromTimeSlot(ctx, timeSlot, isSleepTimerRequest);

    if (timer) {
        if (timer->Duration) {
            if (ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
                timerSetDirective->SetDuration(timer->Duration->Seconds());
            } else {
                timerSetDirective->SetTimestamp(timer->Duration->Seconds());
            }

            time = GetTimeByDuration(*(timer->Duration));
        } else if (timer->Timestamp){
            ctx.Renderer().AddAttention(ATTENTION_TIMER_ABS_TIME);
            timerSetDirective->SetTimestamp(timer->Timestamp->Seconds());
        }
    } else {
        return RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NNlgTemplateNames::BAD_ARGUMENTS
        );
    }

    if (ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
        if (isSleepTimerRequest) {
            AddSleepDirectives(timerSetDirective);

            if (auto sleepTimerId = TryGetSleepTimerId(ctx); sleepTimerId) {
                NScenarios::TDirective directive;
                auto* cancelTimerDirective = directive.MutableCancelTimerDirective();
                cancelTimerDirective->SetName("cancel_timer");
                cancelTimerDirective->SetTimerId(*sleepTimerId);
                ctx.Renderer().AddDirective(std::move(directive));
            }
        } else {
            timerSetDirective->SetListeningIsPossible(true);
        }

        ctx.Renderer().AddDirective(std::move(directive));

        if (!timer->Timestamp && timer->Duration->Seconds() > 3610) { // one hour + time of tts
            if (ctx.HasScledDisplay()) {
                ctx.Renderer().AddScledAnimationTimerTimeDirective(*timer->Duration);
            }
        }

        ctx.Renderer().AddTtsPlayPlaceholderDirective();

    // Searchapp client use timerSetDirective.Timestamp instead timerSetDirective.Duration
    } else {
        if (isSleepTimerRequest) {
            ctx.Renderer().SetIrrelevant();
            return ConstructResponse(ctx);
        }

        *timerSetDirective->MutableOnSuccessCallbackPayload() = MakeTimerSetPayload(true /* success */, frame);
        *timerSetDirective->MutableOnFailureCallbackPayload() = MakeTimerSetPayload(false /* success */, frame);
        timerSetDirective->SetListeningIsPossible(true);

        ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_TIMER_NEED_CONFIRMATION);
        ctx.Renderer().AddDirective(std::move(directive));
    }

    NJson::TJsonValue cardData;
    cardData["time"] = time;
    if (isSleepTimerRequest) {
        cardData["specification"] = "sleep";
    }

    ctx.Renderer().AddVoiceCard(
        nlgTemplate,
        NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    return ConstructResponse(ctx);
}

void TimerCancel(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
) {
    SetTimerAnalyticsInfo(
        ctx,
        frameName,
        isSleepTimerRequest
    );

    if (!IsEnabled(ctx, nlgTemplate)) {
        return;
    }

    if (TryShowTimersForMobile(ctx, nlgTemplate)) {
        return;
    }


    for (auto timer : GetDeviceTimers(ctx, isSleepTimerRequest)) {
        if (timer.GetCurrentlyPlaying()) {
            ctx.Renderer().SetIrrelevant();
            return ConstructResponse(ctx);
        }
    }

    auto frame = ctx.RunRequest().Input().CreateRequestFrame(frameName);
    FillSlotsFromState(frame, ctx.State());

    return ConstructResponseForTimerStateModification(
        ctx,
        frame,
        isSleepTimerRequest,
        CancelTimerAction,
        /* canAction = */ [](const NAlice::TDeviceState::TTimers::TTimer&) { return true; },
        nlgTemplate
    );
}

void TimerPause(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
) {
    SetTimerAnalyticsInfo(
        ctx,
        frameName,
        isSleepTimerRequest
    );

    if (!IsEnabled(ctx, nlgTemplate)) {
        return;
    }

    if (TryShowTimersForMobile(ctx, nlgTemplate)) {
        return;
    }

    auto frame = ctx.RunRequest().Input().CreateRequestFrame(frameName);
    FillSlotsFromState(frame, ctx.State());

    return ConstructResponseForTimerStateModification(
        ctx,
        frame,
        isSleepTimerRequest,
        PauseTimerAction,
        [](const NAlice::TDeviceState::TTimers::TTimer& timer) { return !timer.GetPaused(); },
        nlgTemplate
    );
}

void TimerResume(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
) {
    SetTimerAnalyticsInfo(
        ctx,
        frameName,
        isSleepTimerRequest
    );

    if (!IsEnabled(ctx, nlgTemplate)) {
        return;
    }

    if (TryShowTimersForMobile(ctx, nlgTemplate)) {
        return;
    }

    auto frame = ctx.RunRequest().Input().CreateRequestFrame(frameName);
    FillSlotsFromState(frame, ctx.State());

    return ConstructResponseForTimerStateModification(
        ctx,
        frame,
        isSleepTimerRequest,
        ResumeTimerAction,
        [](const NAlice::TDeviceState::TTimers::TTimer& timer) { return timer.GetPaused(); },
        nlgTemplate
    );
}

void TimerHowLong(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
) {
    SetTimerAnalyticsInfo(
        ctx,
        frameName,
        isSleepTimerRequest
    );

    if (!IsEnabled(ctx, nlgTemplate)) {
        return;
    }

    if (TryShowTimersForMobile(ctx, nlgTemplate)) {
        return;
    }

    auto timers = GetDeviceTimers(ctx, isSleepTimerRequest);

    if (timers.empty()) {
        return RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NNlgTemplateNames::NO_TIMERS
        );
    }

    NJson::TJsonValue cardData;
    cardData["how_long"] = GetTimeByDuration(TDuration::Seconds(timers[0].GetRemaining()));
    if (isSleepTimerRequest) {
        cardData["specification"] = "sleep";
    }

    ctx.Renderer().AddVoiceCard(
        nlgTemplate,
        NAlice::NHollywood::NReminders::NNlgTemplateNames::RENDER_RESULT,
        cardData
    );

    return ConstructResponse(ctx);
}

void TimerShow(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
) {
    SetTimerAnalyticsInfo(
        ctx,
        frameName,
        isSleepTimerRequest
    );

    if (!IsEnabled(ctx, nlgTemplate)) {
        return;
    }

    if (TryShowTimersForMobile(ctx, nlgTemplate)) {
        return;
    }

    auto frame = ctx.RunRequest().Input().CreateRequestFrame(frameName);

    if (auto sleepSpecification = frame.FindSlot(NSlots::SLOT_SLEEP_SPECIFICATION); !IsSlotEmpty(sleepSpecification)) {
        isSleepTimerRequest = true;
    }

    FillSlotsFromState(frame, ctx.State());

    auto timers = GetDeviceTimers(ctx, isSleepTimerRequest);

    if (timers.empty()) {
        return RaiseErrorShowPromo(
            ctx,
            nlgTemplate,
            NNlgTemplateNames::NO_TIMERS
        );
    }

    return ShowMultipleTimers(
        ctx,
        frame,
        nlgTemplate,
        timers
    );
}

void TimerStopPlaying(
    TRemindersContext& ctx,
    const TStringBuf& frameName
) {
    SetTimerAnalyticsInfo(
        ctx,
        frameName,
        /* isSleepTimerRequest = */ false
    );

    if (!ClientSupportsTimers(ctx)) {
        ctx.Renderer().SetIrrelevant();
        return ConstructResponse(ctx);
    }

    bool playingTimerFound = false;
    for (auto timer : GetDeviceTimers(ctx, /* isSleepTimerRequest = */ false)) {
        if (timer.GetCurrentlyPlaying()) {
            NScenarios::TDirective directive;
            auto* timerStopPlayingDirective = directive.MutableTimerStopPlayingDirective();
            timerStopPlayingDirective->SetName("timer_stop_playing");
            timerStopPlayingDirective->SetTimerId(timer.GetTimerId());

            ctx.Renderer().AddDirective(std::move(directive));

            playingTimerFound = true;
        }
    }

    if (!playingTimerFound) {
        ctx.Renderer().SetIrrelevant();
    }

    return ConstructResponse(ctx);
}

} // namespace NAlice::NHollywood::NReminders
