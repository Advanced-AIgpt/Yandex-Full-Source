#include "alarm_prepare.h"

#include <alice/hollywood/library/scenarios/alarm/cases/alarm_cases.h>

namespace NAlice::NHollywood::NReminders {

void TAlarmPrepareHandle::Do(TScenarioHandleContext& scenarioCtx) const {
    TRemindersContext ctx{scenarioCtx};
    if (!ctx.RunRequest().Input().FindSemanticFrame(NFrameNames::ALARM_SET) &&
        !ctx.RunRequest().Input().FindSemanticFrame(NFrameNames::ALARM_SET_ALICE_SHOW)
    ) {
        if (CanProccessFrame(ctx, NFrameNames::ALARM_SET_SOUND)) {
            AlarmPrepareSetSound(ctx, NFrameNames::ALARM_SET_SOUND);
        }
    }
}

} // namespace NAlice::NHollywood::NReminders
