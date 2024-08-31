#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/json/json.h>

#include <util/generic/strbuf.h>


namespace NAlice::NHollywood {

// Stop commands.
constexpr TStringBuf SOUND_LOUDER_COMMAND = "sound_louder";
constexpr TStringBuf SOUND_LOUDER_COMMAND_ELLIPSIS = "sound_louder__ellipsis";
constexpr TStringBuf SOUND_MUTE_COMMAND = "sound_mute";
constexpr TStringBuf SOUND_QUITER_COMMAND = "sound_quiter";
constexpr TStringBuf SOUND_QUITER_COMMAND_ELLIPSIS = "sound_quiter_ellipsis";
constexpr TStringBuf SOUND_SET_LEVEL_COMMAND = "sound_set_level";
constexpr TStringBuf SOUND_UNMUTE_COMMAND = "sound_unmute";

// Nlg names.
constexpr TStringBuf RENDER_SOUND_COMMON_ERROR = "common_error";
constexpr TStringBuf RENDER_SOUND_ERROR = "render_error__sounderror";
constexpr TStringBuf RENDER_SOUND_NOT_SUPPORTED = "render_error__notsupported";
constexpr TStringBuf RENDER_ABSOLUTE_SET_LEVEL_ERROR = "render_error__absolute_set_level";
constexpr TStringBuf SOUND_COMMON_NLG = "sound_common";
constexpr TStringBuf SOUND_MUTE_NLG = "sound_mute";
constexpr TStringBuf SOUND_SET_LEVEL_NLG = "sound_set_level";
constexpr TStringBuf SOUND_UNMUTE_NLG = "sound_unmute";

// Frames.
constexpr TStringBuf LOUDER_FRAME = "personal_assistant.scenarios.sound.louder";
constexpr TStringBuf LOUDER_IN_CONTEXT_FRAME = "personal_assistant.scenarios.sound.louder_in_context";
constexpr TStringBuf QUITER_FRAME = "personal_assistant.scenarios.sound.quiter";
constexpr TStringBuf QUITER_IN_CONTEXT_FRAME = "personal_assistant.scenarios.sound.quiter_in_context";
constexpr TStringBuf MUTE_FRAME = "personal_assistant.scenarios.sound.mute";
constexpr TStringBuf UNMUTE_FRAME = "personal_assistant.scenarios.sound.unmute";
constexpr TStringBuf GET_LEVEL_FRAME = "personal_assistant.scenarios.sound.get_level";
constexpr TStringBuf SET_LEVEL_FRAME = "personal_assistant.scenarios.sound.set_level";
constexpr TStringBuf SET_LEVEL_IN_CONTEXT_FRAME = "personal_assistant.scenarios.sound.set_level_in_context";

// Directives.
constexpr TStringBuf SOUND_SET_LEVEL_DIRECTIVE = "sound_set_level";
constexpr TStringBuf SOUND_GET_LEVEL_DIRECTIVE = "sound_get_level";
constexpr TStringBuf SOUND_MUTE_DIRECTIVE = "sound_mute";
constexpr TStringBuf SOUND_UNMUTE_DIRECTIVE = "sound_unmute";
constexpr TStringBuf SOUND_LOUDER_DIRECTIVE = "sound_louder";
constexpr TStringBuf SOUND_QUITER_DIRECTIVE = "sound_quiter";
constexpr TStringBuf TTS_PLAY_PLACEHOLDER = "tts_play_placeholder";

// Nlu hints.
constexpr TStringBuf SOUND_LOUDER_NLU_HINT_FRAME_NAME = "fast_command.sound_louder__ellipsis";
constexpr TStringBuf SOUND_QUITER_NLU_HINT_FRAME_NAME = "fast_command.sound_quiter__ellipsis";
constexpr TStringBuf SOUND_SET_LEVEL_NLU_HINT_FRAME_NAME = "fast_command.sound_set_level__ellipsis";
constexpr TStringBuf SOUND_LOUDER_NLU_HINT_ACTION_ID = "fast_command.sound_louder__ellipsis_action";
constexpr TStringBuf SOUND_QUITER_NLU_HINT_ACTION_ID = "fast_command.sound_quiter__ellipsis_action";
constexpr TStringBuf SOUND_SET_LEVEL_NLU_HINT_ACTION_ID = "fast_command.sound_set_level__ellipsis_action";

// Analytics info tags
constexpr TStringBuf SOUND_MUTE_INTENT = "personal_assistant.scenarios.sound_mute";
constexpr TStringBuf SOUND_UNMUTE_INTENT = "personal_assistant.scenarios.sound_unmute";
constexpr TStringBuf SOUND_LOUDER_INTENT = "personal_assistant.scenarios.sound_louder";
constexpr TStringBuf SOUND_QUITER_INTENT = "personal_assistant.scenarios.sound_quiter";
constexpr TStringBuf SOUND_SET_INTENT = "personal_assistant.scenarios.sound_set_level";
constexpr TStringBuf SOUND_GET_INTENT = "personal_assistant.scenarios.sound_get_level";

constexpr TStringBuf SOUND_SET_LEVEL_IN_CONTEXT_ACTION_ID = "personal_assistant.scenarios.sound.sel_level_in_context__action";
constexpr TStringBuf SOUND_SET_LEVEL_IN_CONTEXT_ACTION_NAME = "personal_assistant.scenarios.sound.sel_level_in_context";
constexpr TStringBuf SOUND_QUITER_IN_CONTEXT_ACTION_ID = "personal_assistant.scenarios.sound.quiter_in_context__action";
constexpr TStringBuf SOUND_QUITER_IN_CONTEXT_ACTION_NAME = "personal_assistant.scenarios.sound.quiter_in_context";
constexpr TStringBuf SOUND_LOUDER_IN_CONTEXT_ACTION_ID = "personal_assistant.scenarios.sound.louder_in_context__action";
constexpr TStringBuf SOUND_LOUDER_IN_CONTEXT_ACTION_NAME = "personal_assistant.scenarios.sound.louder_in_context";

// Helpers.
void AddMultiroomSessionIdToDirectiveValue(NJson::TJsonValue& directiveValue, const TDeviceState& deviceState);
TMaybe<TFrame> GetFrame(const TScenarioInputWrapper& input, const TVector<TStringBuf>& frames);

} // namespace NAlice::NHollywood
