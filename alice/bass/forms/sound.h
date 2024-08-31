#pragma once

#include "vins.h"

namespace NBASS {

namespace NSound {

inline constexpr TStringBuf LOUDER = "personal_assistant.scenarios.sound_louder";
inline constexpr TStringBuf LOUDER_ELLIPSIS = "personal_assistant.scenarios.sound_louder__ellipsis";
inline constexpr TStringBuf QUITER = "personal_assistant.scenarios.sound_quiter";
inline constexpr TStringBuf QUITER_ELLIPSIS = "personal_assistant.scenarios.sound_quiter__ellipsis";
inline constexpr TStringBuf MUTE = "personal_assistant.scenarios.sound_mute";
inline constexpr TStringBuf UNMUTE = "personal_assistant.scenarios.sound_unmute";
inline constexpr TStringBuf SET_LEVEL = "personal_assistant.scenarios.sound_set_level";
inline constexpr TStringBuf GET_LEVEL = "personal_assistant.scenarios.sound_get_level";
inline constexpr TStringBuf GET_LEVEL_ELLIPSIS = "personal_assistant.scenarios.sound_get_level__ellipsis";

using TLevelDirectiveAdder = std::function<void(TContext&,i64)>;

template<typename TDirective>
TLevelDirectiveAdder MakeLevelDirectiveAdder(TStringBuf type, bool beforeTts = false) {
    return [type, beforeTts](TContext& ctx, i64 level) {
        NSc::TValue data;
        data["new_level"] = level;
        ctx.AddCommand<TDirective>(type, data, beforeTts);
    };
}

void SetLevel(TContext& ctx, const TLevelDirectiveAdder& levelDirAdder, bool allowMute = false, bool errorOnSameLevel = false, i64 currentSoundLevel = 0);

} // namespace NSound

class TSoundFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // namespace NBASS
