#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/sound/sound_common.h>
#include <alice/hollywood/library/multiroom/multiroom.h>

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood {

constexpr TStringBuf EXP_STROKA_YABRO = "stroka_yabro";
constexpr TStringBuf EXP_COMMANDS_MULTIROOM_REDIRECT = "commands_multiroom_redirect";
constexpr TStringBuf EXP_COMMANDS_MULTIROOM_CLIENT_REDIRECT = "commands_multiroom_client_redirect";
constexpr TStringBuf EXP_CLOCK_FACE_CONTROL_TURN_OFF = "clock_face_control_turn_off";
constexpr TStringBuf EXP_CLOCK_FACE_CONTROL_TURN_ON = "clock_face_control_turn_on";
constexpr TStringBuf EXP_CLOCK_FACE_CONTROL_DISABLE_TIME_SLOT_ANTITRIGGER = "clock_face_control_disable_time_slot_antitrigger";
constexpr TStringBuf EXP_CLOCK_FACE_CONTROL_UNSUPPORTED_OPERATION_NLG_RESPONSE = "clock_face_control_unsupported_operation_nlg_response";

// Stop commands.
constexpr TStringBuf ALARM_STOP_COMMAND = "alarm_stop";
constexpr TStringBuf ALARM_STOP_ON_QUASAR_COMMAND = "alarm_stop_on_quasar";
constexpr TStringBuf AUTO_MEDIA_CONTROL_COMMAND = "yandexauto_media_control";
constexpr TStringBuf AUTO_SOUND_COMMAND = "yandexauto_sound";
constexpr TStringBuf AUTO_SOUND_COMMAND_ELLIPSIS = "yandexauto_sound_ellipsis";
constexpr TStringBuf CAR_MEDIA_CONTROL = "car_media_control";
constexpr TStringBuf CAR_VOLUME_DOWN = "car_volume_down";
constexpr TStringBuf CAR_VOLUME_UP = "car_volume_up";
constexpr TStringBuf DO_NOT_DISTURB_ON_FRAME = "alice.do_not_disturb_on";
constexpr TStringBuf DO_NOT_DISTURB_OFF_FRAME = "alice.do_not_disturb_off";
constexpr TStringBuf GC_PAUSE_COMMAND = "general_conversation_player_pause";
constexpr TStringBuf MEDIA_SESSION_PLAY_FRAME = "alice.media_session.play";
constexpr TStringBuf MEDIA_SESSION_PAUSE_FRAME = "alice.media_session.pause";
constexpr TStringBuf NAVI_CONFIRM_COMMAND = "navi_external_confirmation";
constexpr TStringBuf NAVI_SET_SETTINGS_COMMAND = "navi_set_setting";
constexpr TStringBuf PAUSE_FRAME = "personal_assistant.scenarios.player.pause";
constexpr TStringBuf PLAYER_PAUSE_COMMAND = "player_pause";
constexpr TStringBuf POWER_OFF_FRAME = "personal_assistant.stroka.power_off";
constexpr TStringBuf TIMER_STOP_COMMAND = "timer_stop_playing";
constexpr TStringBuf YANDEX_NAVI_EXTERNAL_CONFIRMATION = "yandexnavi_external_confirmation";
constexpr TStringBuf GO_HOME_FRAME = "personal_assistant.scenarios.quasar.go_home";

// Nlg names.
constexpr TStringBuf PLAYER_PAUSE_NLG = "pause_command";
constexpr TStringBuf RENDER_IS_NOT_SMART_SPEAKER = "render_is_not_smart_speaker";
constexpr TStringBuf RENDER_NAVIGATOR_CANCEL_CONFIRMATION = "render_navigator_cancel_confirmation";
constexpr TStringBuf RENDER_PLAYER_PAUSE = "render_player_pause";
constexpr TStringBuf RENDER_UNSUPPORTED_OPERATION = "render_unsupported_operation";
constexpr TStringBuf RENDER_DONT_KNOW_PLACE = "render_dont_know_place";

constexpr TStringBuf POWER_OFF_NLG = "power_off";
constexpr TStringBuf RENDER_POWER_OFF = "render_power_off";

constexpr TStringBuf RENDER_RESULT = "render_result";

constexpr TStringBuf WAITING_FOR_ROUTE_NAVIGATOR_STATE = "waiting_for_route_confirmation";

constexpr TStringBuf CLOCK_FACE_NLG = "clock_face";
constexpr TStringBuf RENDER_CLOCK_FACE_TURN_OFF_UNSUPPORTED_OPERATION = "render_unsupported_turn_off_operation";
constexpr TStringBuf RENDER_CLOCK_FACE_TURN_ON_UNSUPPORTED_OPERATION = "render_unsupported_turn_on_operation";
constexpr TStringBuf RENDER_CLOCK_FACE_ALREADY_TURNED_OFF = "render_already_turned_off";

// Directives.
constexpr TStringBuf ALARM_STOP_DIRECTIVE = "alarm_stop";
constexpr TStringBuf AUDIO_STOP_DIRECTIVE = "audio_stop";
constexpr TStringBuf CAR_DIRECTIVE = "car";
constexpr TStringBuf CLEAR_QUEUE_DIRECTIVE = "clear_queue";
constexpr TStringBuf OPEN_URI_DIRECTIVE = "open_uri";
constexpr TStringBuf PLAYER_PAUSE_DIRECTIVE = "player_pause";
constexpr TStringBuf POWER_OFF_DIRECTIVE = "power_off";
constexpr TStringBuf POWER_OFF_DIRECTIVE_SUB_NAME = "pc_power_off";
constexpr TStringBuf START_MULTIROOM_DIRECTIVE = "start_multiroom";
constexpr TStringBuf TIMER_STOP_DIRECTIVE = "timer_stop_playing";
constexpr TStringBuf YANDEX_NAVI_DIRECTIVE = "yandexnavi";

// Analytics info tags
constexpr TStringBuf ALARM_STOP_INTENT = "personal_assistant.scenarios.alarm_stop_playing";
constexpr TStringBuf GO_HOME_INTENT = "personal_assistant.scenarios.quasar.go_home";
constexpr TStringBuf PLAYER_PAUSE_INTENT = "personal_assistant.scenarios.player_pause";
constexpr TStringBuf POWER_OFF_INTENT = "personal_assistant.stroka.power_off";
constexpr TStringBuf TIMER_STOP_INTENT = "personal_assistant.scenarios.timer_stop_playing";

// Helpers.
void FillAnalyticsInfo(TResponseBodyBuilder& bodyBuilder, TStringBuf intent, const TString& productScenarioName);

void AddRedirectToLocation(NJson::TJsonValue& directiveValue, const TFrame& frame);

} // namespace NAlice::NHollywood
