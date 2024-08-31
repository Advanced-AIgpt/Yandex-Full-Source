#pragma once

#include <alice/hollywood/library/scenarios/alarm/context/context.h>
#include <alice/hollywood/library/scenarios/alarm/util/util.h>

namespace NAlice::NHollywood::NReminders {

void TimerSet(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
);

void TimerCancel(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
);

void TimerPause(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
);

void TimerResume(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
);

void TimerHowLong(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
);

void TimerShow(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName,
    bool isSleepTimerRequest
);

void TimerStopPlaying(
    TRemindersContext& ctx,
    const TStringBuf& frameName
);

} // namespace NAlice::NHollywood::NReminders
