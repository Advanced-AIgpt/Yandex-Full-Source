#pragma once

#include <alice/hollywood/library/scenarios/alarm/context/context.h>
#include <alice/hollywood/library/scenarios/alarm/util/util.h>

namespace NAlice::NHollywood::NReminders {

void SetIrrelevant(TRemindersContext& ctx);

void AlarmPlayAliceShow(TRemindersContext& ctx);

void AlarmSetAliceShow(
    TRemindersContext& ctx,
    TStringBuf nlgTemplate = NNlgTemplateNames::ALARM_SET_ALICE_SHOW,
    bool constructResponse = true
);

void AlarmSetWithAliceShow(TRemindersContext& ctx);

//TODO(danibw) uncomment in MEGAMIND-2290
void AlarmSet(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& intent,
    bool checkRelocationFlag = true
);

void AlarmCancel(
    TRemindersContext& ctx,
    const TStringBuf intent
);

void AlarmHowLong(TRemindersContext& ctx);

void AlarmStopPlaying(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName
);

void AlarmShow(TRemindersContext& ctx);
//void AlarmSetSound(TRemindersContext& ctx);
void AlarmResetSound(TRemindersContext& ctx);
void AlarmWhatSoundIsSet(TRemindersContext& ctx);
//void AlarmSetWithSound(TRemindersContext& ctx);
void AlarmSoundSetLevel(TRemindersContext& ctx);
void AlarmWhatSoundLevelIsSet(TRemindersContext& ctx);

void AlarmFallback(TRemindersContext& ctx);

void AlarmPrepareSetSound(
    TRemindersContext& ctx,
    const TStringBuf& frameName
);

void AlarmSetSound(
    TRemindersContext& ctx,
    const TStringBuf frameName
);

void AlarmHowToSetSound(
    TRemindersContext& ctx,
    const TStringBuf& nlgTemplate,
    const TStringBuf& frameName
);

} // namespace NAlice::NHollywood::NReminders
