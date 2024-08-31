#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <library/cpp/scheme/fwd.h>

#include <util/generic/strbuf.h>

namespace NBASS {
namespace NReminders {
/* all the documentation is here:
 * VinsBass: https://wiki.yandex-team.ru/assistant/dialogs/timer/Vins-Bass-protokol/
 * ServerActions: https://wiki.yandex-team.ru/quasar/timers-protocol/
 */

inline constexpr TStringBuf TIMER_FORM_NAME_SET = "personal_assistant.scenarios.timer_set";
inline constexpr TStringBuf TIMER_FORM_NAME_SET_WITH_ACTION = "personal_assistant.scenarios.timer_with_action_set";
inline constexpr TStringBuf TIMER_FORM_NAME_SET_WITH_ACTION_ELLIPSIS = "personal_assistant.scenarios.timer_with_action_set__ellipsis";

enum class ETimerType {
    Normal /* "normal" */,    // Usual good old timers - they alarm when time is elapsed.
    Sleep /* "sleep" */       // Sleep timers for smart speakers - they stop whatever is playing (like music, radio, video or tv) when time is elapsed.
};

void TimerSet(TContext& ctx);
void TimerShow(TContext& ctx);
void TimerCancel(TContext& ctx);
void TimerPause(TContext& ctx);
void TimerResume(TContext& ctx);
void TimerStopPlaying(TContext& ctx);
void TimerHowLong(TContext& ctx);
void SleepTimerSet(TContext& ctx);
void SleepTimerHowLong(TContext& ctx);

namespace NImpl {
bool AreJsonDirectivesEqualByName(const NSc::TValue& lhs, const NSc::TValue& rhs);
} // namespace NImpl

} // namespace NReminders
} // namespace NBASS
