#include "automotive/sound.h"
#include "directives.h"
#include "navigator/set_sound.h"
#include "sound.h"
#include "music/providers.h"
#include "radio.h"
#include "player/player.h"
#include "player_command.h"

#include <alice/library/analytics/common/product_scenarios.h>


namespace NBASS {

namespace {

const int MAX_VOLUME = 10;
const int VERY_HIGH_VOLUME = 9;
const int HIGH_VOLUME = 8;
const int MID_VOLUME = 4;
const int QUIET_VOLUME = 2;
const int VERY_QUIET_VOLUME = 1;
const int MIN_SOUNDING_VOLUME = 1;
const int MIN_VOLUME = 0;

static const THashMap<TString, int> VOLUME_MAP = {
    {"minimum", MIN_SOUNDING_VOLUME},
    {"very_quiet", VERY_QUIET_VOLUME},
    {"quiet", QUIET_VOLUME},
    {"middle", MID_VOLUME},
    {"high", HIGH_VOLUME},
    {"very_high", VERY_HIGH_VOLUME},
    {"maximum", MAX_VOLUME},
};

constexpr TStringBuf TYPE_NUM = "num";
constexpr TStringBuf TYPE_VOLUME_SETTING = "volume_setting";

void AddSoundError(TContext& ctx, TStringBuf code) {
    ctx.AddErrorBlockWithCode(TError::EType::SOUNDERROR, code);
}

void AddLevelNumSlot(TContext& ctx, i64 num) {
    ctx.CreateSlot(TStringBuf("level"), TYPE_NUM, /* optional= */ true, num);
}

} // namespace

namespace NSound {

void SetLevel(TContext& ctx, const TLevelDirectiveAdder& levelDirAdder, bool allowMute, bool errorOnSameLevel, i64 currentSoundLevel) {
    // Like "set volume 7"
    if (const auto* const levelSlot = ctx.GetSlot("level", TYPE_NUM);
        !IsSlotEmpty(levelSlot))
    {
        const i64 level = levelSlot->Value.GetIntNumber();
        if (level < (allowMute ? MIN_VOLUME : MIN_SOUNDING_VOLUME) || level > MAX_VOLUME) {
            AddSoundError(ctx, TStringBuf("level_out_of_range"));
        } else {
            levelDirAdder(ctx, level);
        }
        return;
    }

    // Like "set volume to max"
    if (const auto* const levelSlot = ctx.GetSlot("level", TYPE_VOLUME_SETTING);
        !IsSlotEmpty(levelSlot))
    {
        if (const auto* level = VOLUME_MAP.FindPtr(levelSlot->Value.GetString())) {
            if (errorOnSameLevel && currentSoundLevel == *level) {
                AddSoundError(ctx, MAX_VOLUME == *level ? "already_max" : "already_set");
            } else {
                levelDirAdder(ctx, *level);
                AddLevelNumSlot(ctx, *level);
            }
        } else {
            AddSoundError(ctx, TStringBuf("undefined_level_limit"));
        }
        return;
    }

    ctx.AddErrorBlockWithCode(TError::EType::INVALIDPARAM, TStringBuf("unknown_or_empty_slot"));
}

} // namespace NSound

TResultValue TSoundFormHandler::Do(TRequestHandler &r) {
    TContext &Ctx = r.Ctx();
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SOUND_COMMAND);

    if (r.Ctx().MetaClientInfo().IsNavigator()) {
        TSoundSettingsNavigatorIntent navigatorIntent(r.Ctx(), NSound::MUTE, NSound::UNMUTE);
        return navigatorIntent.Do();
    }

    if (r.Ctx().MetaClientInfo().IsYaAuto()) {
        return NAutomotive::HandleSound(Ctx);
    }

    i64 currentSoundLevel = Ctx.Meta().DeviceState().SoundLevel();

    if (!Ctx.ClientFeatures().SupportsAnyPlayer() || currentSoundLevel == -1) {
        Ctx.AddErrorBlock(TError::EType::NOTSUPPORTED);
        return TResultValue();
    }

    bool isPlayingAudio = !NPlayer::AreAudioPlayersPaused(Ctx);
    if (isPlayingAudio) {
        Ctx.AddAttention("is_playing_audio");
    }

    if (Ctx.FormName() == NSound::MUTE) {
        Ctx.AddCommand<TSoundMuteDirective>(TStringBuf("sound_mute"), NSc::Null());
    } else if (Ctx.FormName() == NSound::UNMUTE) {
        Ctx.AddCommand<TSoundUnmuteDirective>(TStringBuf("sound_unmute"), NSc::Null());
    } else if (Ctx.FormName() == NSound::LOUDER || Ctx.FormName() == NSound::LOUDER_ELLIPSIS) {
        Ctx.AddCommand<TSoundLoaderDirective>(TStringBuf("sound_louder"), NSc::Null());
        if (currentSoundLevel >= MAX_VOLUME) {
            AddSoundError(Ctx, TStringBuf("already_max"));
        }
    } else if (Ctx.FormName() == NSound::QUITER || Ctx.FormName() == NSound::QUITER_ELLIPSIS) {
        Ctx.AddCommand<TSoundQuiterDirective>(TStringBuf("sound_quiter"), NSc::Null());
        if (currentSoundLevel <= MIN_VOLUME) {
            AddSoundError(Ctx, TStringBuf("already_min"));
        }
    } else if (Ctx.FormName() == NSound::SET_LEVEL || Ctx.FormName() == NSound::GET_LEVEL_ELLIPSIS) {
        NSound::SetLevel(
            Ctx, NSound::MakeLevelDirectiveAdder<TSoundSetLevelDirective>(TStringBuf("sound_set_level")),
            /* allowMute= */ true, /* errorOnSameLevel= */ true, currentSoundLevel
        );
    } else if (Ctx.FormName() == NSound::GET_LEVEL) {
        AddLevelNumSlot(Ctx, currentSoundLevel);
    }

    return TResultValue();
}

void TSoundFormHandler::Register(THandlersMap *handlers) {
    auto cbSoundForm = []() {
        return MakeHolder<TSoundFormHandler>();
    };
    handlers->emplace(NSound::LOUDER, cbSoundForm);
    handlers->emplace(NSound::LOUDER_ELLIPSIS, cbSoundForm);
    handlers->emplace(NSound::MUTE, cbSoundForm);
    handlers->emplace(NSound::QUITER, cbSoundForm);
    handlers->emplace(NSound::QUITER_ELLIPSIS, cbSoundForm);
    handlers->emplace(NSound::UNMUTE, cbSoundForm);
    handlers->emplace(NSound::SET_LEVEL, cbSoundForm);
    handlers->emplace(NSound::GET_LEVEL, cbSoundForm);
    handlers->emplace(NSound::GET_LEVEL_ELLIPSIS, cbSoundForm);
}

} // namespace NBASS
