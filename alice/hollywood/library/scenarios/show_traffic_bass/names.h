#pragma once

#include <util/string/join.h>

namespace NAlice::NHollywood {

namespace NShowTrafficBass {

    inline constexpr TStringBuf FEEDBACK = "feedback";
    const TString PRODUCT_SCENARIO_NAME = "show_traffic";

    const TString POSITIVE_FEEDBACK_INTENT = "personal_assistant.feedback.feedback_positive";
    const TString NEGATIVE_FEEDBACK_INTENT = "personal_assistant.feedback.feedback_negative";
    const TString CONTINUE_FEEDBACK_INTENT = "personal_assistant.feedback.feedback_continue";

    inline constexpr TStringBuf FEEDBACK_OPTIONS_REASONS = "feedback_negative__bad_answer;feedback_negative__asr_error;feedback_negative__tts_error;feedback_negative__offensive_answer;feedback_negative__other;feedback_negative__all_good";
    inline constexpr TStringBuf ONBOARDING_SUGGEST = "onboarding__what_can_you_do";

    inline constexpr TStringBuf SHOW_TRAFFIC_FRAME = "personal_assistant.scenarios.show_traffic";
    inline constexpr TStringBuf SHOW_TRAFFIC_DETAILS_FRAME = "personal_assistant.scenarios.show_traffic__details";
    inline constexpr TStringBuf SHOW_TRAFFIC_ELLIPSIS_FRAME = "personal_assistant.scenarios.show_traffic__ellipsis";
    inline constexpr TStringBuf COLLECT_MAIN_SCREEN_FRAME = "alice.centaur.collect_main_screen";
    inline constexpr TStringBuf COLLECT_WIDGET_GALLERY_FRAME = "alice.centaur.collect_widget_gallery";
    inline constexpr TStringBuf CENTAUR_COLLECT_MAIN_SCREEN_TRAFFIC_SEMANTIC_FRAME = "alice.centaur.collect_main_screen.widgets.traffic";

    inline constexpr TStringBuf SHOW_TRAFFIC_BASS_NLG = "show_traffic_bass";
    inline constexpr TStringBuf SHOW_TRAFFIC_BASS_DETAILS_NLG = "show_traffic_bass__details";

    inline constexpr TStringBuf DISABLE_VOICE_FRAMES = "disable_traffic_voice_frames";

    inline constexpr TStringBuf SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME = "scenario_widget_mechanics";

    const TString CALLBACK_FEEDBACK_NAME = "alice.show_traffic_feedback";

} // namespace NShowTrafficBass

namespace NFeedbackOptions {
    const TString POSITIVE = "feedback_positive_show_traffic";
    const TString NEGATIVE = "feedback_negative_show_traffic";
} // namespace NFeedbackOptions

namespace NNaviDirectives {
    const TString NAVI_SHOW_POINT_ON_MAP = "navi_show_point_on_map";
    const TString NAVI_LAYER_TRAFFIC = "navi_layer_traffic";
    const TString SHOW_TRAFFIC_LAYER = "yandexnavi://traffic?traffic_on=1";
} // namespace NNaviDirectives

} // namespace NAlice::NHollywood
