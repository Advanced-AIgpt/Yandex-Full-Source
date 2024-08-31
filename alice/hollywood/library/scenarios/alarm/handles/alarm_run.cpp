#include "alarm_run.h"
#include <alice/hollywood/library/scenarios/alarm/cases/alarm_cases.h>
#include <alice/hollywood/library/scenarios/alarm/cases/timer_cases.h>

namespace NAlice::NHollywood::NReminders {

void TAlarmRunHandle::Do(TScenarioHandleContext& scenarioCtx) const {
    TRemindersContext ctx{scenarioCtx};

    if (const auto* callback = ctx.RunRequest().Input().GetCallback(); callback) {
        if (callback->GetName() == NFrameNames::ALARM_PLAY_MORNING_SHOW) {
            AlarmPlayAliceShow(ctx);
        } else if (auto callbackUpdateFormFrame = TryGetCallbackUpdateFormFrame(callback); callbackUpdateFormFrame) {
            if (callbackUpdateFormFrame->Name() == NFrameNames::ALARM_SET) {
                AlarmSet(ctx, NNlgTemplateNames::ALARM_SET, NFrameNames::ALARM_SET);
            } else if (callbackUpdateFormFrame->Name() == NFrameNames::ALARM_ASK_TIME) {
                AlarmSet(ctx, NNlgTemplateNames::ALARM_SET, NFrameNames::ALARM_ASK_TIME);
            } else if (callbackUpdateFormFrame->Name() == NFrameNames::TIMER_SET) {
                TimerSet(ctx, NNlgTemplateNames::TIMER_SET, NFrameNames::TIMER_SET, /* isSleepTimerRequest */ false);
            } else if (callbackUpdateFormFrame->Name() == NFrameNames::TIMER_SET__ELLIPSIS) {
                TimerSet(ctx, NNlgTemplateNames::TIMER_SET, NFrameNames::TIMER_SET__ELLIPSIS, /* isSleepTimerRequest */ false);
            } else {
                SetIrrelevant(ctx);
            }
        } else {
            SetIrrelevant(ctx);
        }
    } else {
        if (CanProccessFrame(ctx, NFrameNames::ALARM_CANCEL__ELLIPSIS)) {
            AlarmCancel(ctx, NFrameNames::ALARM_CANCEL__ELLIPSIS);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_CANCEL)) {
            AlarmCancel(ctx, NFrameNames::ALARM_CANCEL);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SHOW__CANCEL)) {
            AlarmCancel(ctx, NFrameNames::ALARM_SHOW__CANCEL);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_HOW_LONG)) {
            AlarmHowLong(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_HOW_TO_SET_SOUND)) {
            AlarmHowToSetSound(ctx, NNlgTemplateNames::ALARM_HOW_TO_SET_SOUND, NFrameNames::ALARM_HOW_TO_SET_SOUND);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_RESET_SOUND)) {
            AlarmResetSound(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SET_ALICE_SHOW)) {
            AlarmSetAliceShow(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SET_MORNING_SHOW)) { // for backward compatibility
            AlarmSetAliceShow(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SHOW)) {
            AlarmShow(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SNOOZE_ABS)) {
            AlarmSet(ctx, NNlgTemplateNames::ALARM_SET, NFrameNames::ALARM_SNOOZE_ABS);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SNOOZE_REL)) {
            AlarmSet(ctx, NNlgTemplateNames::ALARM_SET, NFrameNames::ALARM_SNOOZE_REL);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_ASK_TIME)) {
            AlarmSet(ctx, NNlgTemplateNames::ALARM_SET, NFrameNames::ALARM_ASK_TIME);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SET)) {
            AlarmSet(ctx, NNlgTemplateNames::ALARM_SET, NFrameNames::ALARM_SET);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_STOP_PLAYING)) {
            AlarmStopPlaying(ctx, NNlgTemplateNames::ALARM_STOP_PLAYING, NFrameNames::ALARM_STOP_PLAYING);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_WHAT_SOUND_IS_SET)) {
            AlarmWhatSoundIsSet(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SET_WITH_ALICE_SHOW)) {
            AlarmSetWithAliceShow(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SET_WITH_MORNING_SHOW)) { // for backward compatibility
            AlarmSetWithAliceShow(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SOUND_SET_LEVEL)) {
            AlarmSoundSetLevel(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_WHAT_SOUND_LEVEL_IS_SET)) {
            AlarmWhatSoundLevelIsSet(ctx);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_CANCEL__ELLIPSIS, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerCancel(ctx, NNlgTemplateNames::TIMER_CANCEL, NFrameNames::TIMER_CANCEL__ELLIPSIS, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_CANCEL, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerCancel(ctx, NNlgTemplateNames::TIMER_CANCEL, NFrameNames::TIMER_CANCEL, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_HOW_LONG, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerHowLong(ctx, NNlgTemplateNames::TIMER_HOW_LONG, NFrameNames::TIMER_HOW_LONG, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_PAUSE__ELLIPSIS, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerPause(ctx, NNlgTemplateNames::TIMER_PAUSE, NFrameNames::TIMER_PAUSE__ELLIPSIS, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_PAUSE, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerPause(ctx, NNlgTemplateNames::TIMER_PAUSE, NFrameNames::TIMER_PAUSE, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_RESUME__ELLIPSIS, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerResume(ctx, NNlgTemplateNames::TIMER_RESUME, NFrameNames::TIMER_RESUME__ELLIPSIS, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_RESUME, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerResume(ctx, NNlgTemplateNames::TIMER_RESUME, NFrameNames::TIMER_RESUME, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_SET__ELLIPSIS, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerSet(ctx, NNlgTemplateNames::TIMER_SET, NFrameNames::TIMER_SET__ELLIPSIS, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_SET, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerSet(ctx, NNlgTemplateNames::TIMER_SET, NFrameNames::TIMER_SET, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_SHOW, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerShow(ctx, NNlgTemplateNames::TIMER_SHOW, NFrameNames::TIMER_SHOW, /* isSleepTimerRequest */ false);
        } else if (CanProccessFrame(ctx, NFrameNames::SLEEP_TIMER_CANCEL, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerCancel(ctx, NNlgTemplateNames::TIMER_CANCEL, NFrameNames::SLEEP_TIMER_CANCEL, /* isSleepTimerRequest */ true);
        } else if (CanProccessFrame(ctx, NFrameNames::SLEEP_TIMER_HOW_LONG, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerHowLong(ctx, NNlgTemplateNames::TIMER_HOW_LONG, NFrameNames::SLEEP_TIMER_HOW_LONG, /* isSleepTimerRequest */ true);
        } else if (CanProccessFrame(ctx, NFrameNames::SLEEP_TIMER_SET__ELLIPSIS, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerSet(ctx, NNlgTemplateNames::TIMER_SET, NFrameNames::SLEEP_TIMER_SET__ELLIPSIS, /* isSleepTimerRequest */ true);
        } else if (CanProccessFrame(ctx, NFrameNames::SLEEP_TIMER_SET, /* checkExpFlagForSmartSpeaker = */ false)) {
            TimerSet(ctx, NNlgTemplateNames::TIMER_SET, NFrameNames::SLEEP_TIMER_SET, /* isSleepTimerRequest */ true);
        } else if (CanProccessFrame(ctx, NFrameNames::TIMER_STOP_PLAYING)) {
            TimerStopPlaying(ctx, NFrameNames::TIMER_STOP_PLAYING);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_SET_SOUND)) {
            AlarmSetSound(ctx, NFrameNames::ALARM_SET_SOUND);
        } else if (CanProccessFrame(ctx, NFrameNames::ALARM_FALLBACK)) {
            AlarmFallback(ctx);
        } else {
            SetIrrelevant(ctx);
        }
    }
}

} // namespace NAlice::NHollywood::NReminders
