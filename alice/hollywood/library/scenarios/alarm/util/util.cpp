#include "util.h"

#include <alice/library/music/catalog.h>
#include <alice/library/proto/protobuf.h>

#include <util/string/printf.h>

namespace {

::google::protobuf::Struct MakeAlarmSetPayload(bool success, const NAlice::NHollywood::TFrame& frame) {
    auto bufFrame = frame;
    auto successSlot = NAlice::NHollywood::NReminders::FindOrAddSlot(
        bufFrame,
        NAlice::NHollywood::NReminders::NSlots::SLOT_ALARM_SET_SUCCESS,
        NAlice::NHollywood::NReminders::NSlots::TYPE_BOOL
    );

    const_cast<NAlice::NHollywood::TSlot*>(successSlot.Get())->Value = NAlice::NHollywood::TSlot::TValue(
        NAlice::JsonToString(NJson::TJsonValue(success))
    );

    return NAlice::NHollywood::NReminders::ToPayloadUpdateForm(bufFrame.ToProto());
}

bool IsSleepTimer(const NAlice::TDeviceState::TTimers::TTimer& timer) {
    return !timer.GetDirectives().empty();
}

} // namespace

namespace NAlice::NHollywood::NReminders {

bool CanProccessFrame(TRemindersContext& ctx, const TStringBuf frameName, bool checkExpFlagForSmartSpeaker) {
    if (!ctx.RunRequest().Input().FindSemanticFrame(frameName)) {
        return false;
    }

    if (auto* expFlagPtr = FRAME_NAME_TO_EXP_FLAG.FindPtr(frameName); expFlagPtr) {
        if (!ctx.RunRequest().ClientInfo().IsSmartSpeaker() || checkExpFlagForSmartSpeaker) {
            if (!ctx.RunRequest().HasExpFlag(*expFlagPtr)) {
                return false;
            }
        }
    }

    if (frameName == NFrameNames::ALARM_SHOW) {
        return !ctx.RunRequest().HasExpFlag(NExperiments::HW_ALARM_RELOCATION__ALARM_SHOW_DISABLED);
    }

    if (frameName == NFrameNames::ALARM_SNOOZE_ABS || frameName == NFrameNames::ALARM_SNOOZE_REL) {
        return ctx.GetAlarmState().GetCurrentlyPlaying();
    }

    if (frameName == NFrameNames::ALARM_CANCEL) {
        return !ctx.GetAlarmState().GetCurrentlyPlaying();
    }

    if (frameName == NFrameNames::ALARM_ASK_TIME) {
        return GetFrameName(ctx.State()) == NFrameNames::ALARM_SET;
    }

    if (frameName == NFrameNames::ALARM_CANCEL__ELLIPSIS) {
        return GetFrameName(ctx.State()) == NFrameNames::ALARM_CANCEL;
    }

    if (frameName == NFrameNames::ALARM_SHOW__CANCEL) {
        return GetFrameName(ctx.State()) == NFrameNames::ALARM_SHOW;
    }

    if (frameName == NFrameNames::TIMER_CANCEL) {
        for (auto timer : GetDeviceTimers(ctx, /* isSleepTimerRequest = */ false)) {
            if (timer.GetCurrentlyPlaying()) {
                return false;
            }
        }
    }

    if (frameName == NFrameNames::TIMER_CANCEL__ELLIPSIS) {
        return GetFrameName(ctx.State()) == NFrameNames::TIMER_CANCEL;
    }

    if (frameName == NFrameNames::TIMER_PAUSE__ELLIPSIS) {
        return GetFrameName(ctx.State()) == NFrameNames::TIMER_PAUSE;
    }

    if (frameName == NFrameNames::TIMER_RESUME__ELLIPSIS) {
        return GetFrameName(ctx.State()) == NFrameNames::TIMER_RESUME;
    }

    if (frameName == NFrameNames::TIMER_SET__ELLIPSIS) {
        return GetFrameName(ctx.State()) == NFrameNames::TIMER_SET;
    }

    if (frameName == NFrameNames::SLEEP_TIMER_SET__ELLIPSIS) {
        return GetFrameName(ctx.State()) == NFrameNames::SLEEP_TIMER_SET;
    }

    return true;
}

bool CheckExpFlag(
    TRemindersContext& ctx,
    const TStringBuf& flag,
    const TStringBuf& nlgTemplateName
) {
    auto& renderer = ctx.Renderer();
    if (!ctx.RunRequest().HasExpFlag(flag)) {
        renderer.SetNotSupported(
            nlgTemplateName
        );

        renderer.SetIrrelevant();
        ConstructResponse(ctx);
        return false;
    }

    return true;
}

TInstant GetCurrentTimestamp(TRemindersContext& ctx) {
    return TInstant::Seconds(ctx.GetEpoch());
}

void ConstructResponse(TRemindersContext& ctx) {
    auto& renderer = ctx.Renderer();
    renderer.Render();
    auto response = *std::move(renderer.Builder()).BuildResponse();
    response.MutableResponseBody()->MutableState()->PackFrom(ctx.State());
    ctx->ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
}

bool IsSlotEmpty(const TPtrWrapper<TSlot>& slot) {
    return !slot || slot->Value.AsString() == "null";
}

TPtrWrapper<TSlot> FindOrAddSlot(
    TFrame& frame,
    const TString& name,
    const TString& type
) {
    TPtrWrapper<TSlot> slot = frame.FindSlot(name);
    if (!slot) {
        frame.AddSlot(TSlot{.Name = name, .Type = type, .Value = TSlot::TValue{"null"}});
        return frame.FindSlot(name);
    }
    return slot;
}

TPtrWrapper<TSlot> FindSlot(
    const TFrame& frame,
    const TString& name,
    const TString& type
) {
    TPtrWrapper<TSlot> slot = frame.FindSlot(name);
    if (slot && slot->Type != type) {
        return TPtrWrapper<TSlot>(nullptr, name);
    }
    return slot;
}

TPtrWrapper<TSlot> AddSlot(
    TFrame& frame,
    const TSlot& slot
) {
    frame.AddSlot(slot);
    return frame.FindSlot(slot.Name);
}

TPtrWrapper<TSlot> FindSingleSlot(
    const TFrame& frame,
    const TString& name
) {
    if (const auto* slots = frame.FindSlots(name)) {
        if (slots->size() == 1) {
            return TPtrWrapper<TSlot>{&slots->front(), name};
        }
    }

    return TPtrWrapper<TSlot>{nullptr, name};
}

bool IsTodayOrTomorrow(
    const TInstant& now,
    const NDatetime::TTimeZone& tz,
    const TMaybe<NScenarios::NAlarm::TDate>& date
) {
    return date && date->HasExactDay() ? date->IsTodayOrTomorrow(NDatetime::Convert(now, tz)) : false;
}

void AddAlarmSetCommand(
    TRemindersContext& ctx,
    const TInstant& now,
    const NDatetime::TTimeZone& tz,
    TVector<NScenarios::NAlarm::TWeekdaysAlarm>& alarms,
    const NScenarios::NAlarm::TWeekdaysAlarm& alarm,
    const TFrame& frame,
    bool isSnooze
) {
    for (const auto& existingAlarm : alarms) {
        if (alarm.IsSubsetOf(existingAlarm)) {
            ctx.Renderer().AddAttention(ATTENTION_ALARM_ALREADY_SET);
            return;
        }
    }

    alarms.push_back(alarm);
    if (ctx.RunRequest().ClientInfo().IsSmartSpeaker()) {
        if (isSnooze) {
            NScenarios::TDirective directive;
            directive.MutableAlarmStopDirective()->SetName("alarm_stop");
            ctx.Renderer().AddDirective(std::move(directive));
        }

        NScenarios::TDirective directive;
        auto* alarmsUpdateDirective = directive.MutableAlarmsUpdateDirective();
        alarmsUpdateDirective->SetState(NScenarios::NAlarm::TWeekdaysAlarm::ToICalendar(alarms));
        alarmsUpdateDirective->SetListeningIsPossible(true);

        ctx.Renderer().AddDirective(std::move(directive));

        if (ctx.HasScledDisplay()) {
            ctx.Renderer().AddScledAnimationAlarmTimeDirective(
                NDatetime::Convert(alarm.Begin, tz),
                NDatetime::Convert(now, tz)
            );
        }

        // tts_play_placeholder should be last one, see ALICE-14090
        ctx.Renderer().AddTtsPlayPlaceholderDirective();
    } else {
        NScenarios::TDirective directive;
        auto* alarmNewDirective = directive.MutableAlarmNewDirective();
        alarmNewDirective->SetState(NScenarios::NAlarm::TWeekdaysAlarm::ToICalendar(alarms));
        *alarmNewDirective->MutableOnSuccessCallbackPayload() = MakeAlarmSetPayload(true /* success */, frame);
        *alarmNewDirective->MutableOnFailureCallbackPayload() = MakeAlarmSetPayload(false /* success */, frame);

        ctx.Renderer().AddDirective(std::move(directive));
        ctx.Renderer().AddAttention(NAlice::NHollywood::NReminders::ATTENTION_ALARM_NEED_CONFIRMATION);
    }

    return;
}

void RaiseErrorShowPromo(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& errorCode,
    const TStringBuf& phraseName
) {

    if (errorCode == NNlgTemplateNames::NOT_SUPPORTED) {
        if (ctx.Renderer().TryAddShowPromoDirective(ctx.GetInterfaces())) {
            LOG_INFO(ctx.Logger()) << "Adding show promo directive" << Endl;
        }
    }    
    ctx.Renderer().AddError(errorCode);
    ctx.Renderer().AddVoiceCard(nlgTemplate, phraseName);

    return ConstructResponse(ctx);
}

void FillSlotsFromState(
    TFrame& frame,
    const TRemindersState& state
) {
    auto stateFrame = TFrame::FromProto(state.GetSemanticFrame());
    for (const auto& stateSlot : stateFrame.Slots()) {
        if (!frame.FindSlot(stateSlot.Name)) {
            frame.AddSlot(stateSlot);
        }
    }
}

TStringBuf GetFrameName(
    const TRemindersState& state
) {
    if (state.HasSemanticFrame()) {
        return state.GetSemanticFrame().GetName();
    }
    return TStringBuf();
}

TMaybe<TFrame> TryGetCallbackUpdateFormFrame(const NScenarios::TCallbackDirective* callback) {
    if (!callback || callback->GetName() != UPDATE_FORM_CALLBACK) {
        return Nothing();
    }
    return TFrame::FromProto(JsonToProto<TSemanticFrame>(
            JsonFromString(callback->GetPayload().fields().at("frame").string_value())));
}

::google::protobuf::Struct ToPayloadUpdateForm(const TSemanticFrame& frame) {
    ::google::protobuf::Struct payload;
    (*payload.mutable_fields())["frame"].set_string_value(JsonStringFromProto(frame));
    return payload;
}

TString ConstructedScledPattern(
    const NDatetime::TCivilSecond& time,
    const TString& pattern
) {
    return Sprintf(pattern.c_str(), time.hour(), time.minute());
}

bool IsFairyTaleFilterGenre(const TFrame& frame) {
    const auto slotIsFairyTaleNew = frame.FindSlot("is_fairy_tale_filter_genre");
    return !IsSlotEmpty(slotIsFairyTaleNew) && slotIsFairyTaleNew->Value.AsString() == "true";
}

TVector<NAlice::TDeviceState::TTimers::TTimer> GetDeviceTimers(
    TRemindersContext& ctx,
    bool isSleepTimerRequest
) {
    TVector<NAlice::TDeviceState::TTimers::TTimer> timers;

    for (const auto& t : ctx.GetTimers().GetActiveTimers()) {
        if (isSleepTimerRequest) {
            if (IsSleepTimer(t)) {
                timers.emplace_back(t);
            }
        } else {
            timers.emplace_back(t);
        }
    }

    Sort(timers.begin(), timers.end(), [](const auto& l, const auto& r) {
        if (l.GetPaused() != r.GetPaused()) {
            return l.GetPaused() < r.GetPaused();
        }
        return l.GetRemaining() < r.GetRemaining();
    });

    return timers;
}

} // namespace NAlice::NHollywood::NReminders
