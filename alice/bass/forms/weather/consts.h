#pragma once

#include <util/generic/strbuf.h>

namespace NBASS::NWeather {

namespace NExperiment {
// New NLG for weather today - https://st.yandex-team.ru/DIALOG-5039
inline constexpr TStringBuf DISABLE_NEW_NLG = "disable_new_nlg"; // Disables the new NLG without comparing to yesterday
inline constexpr TStringBuf NEW_NLG_COMPARE = "new_nlg_compare"; // Enables the new NLG with comparing to yesterday

// Experiments with suggests — https://st.yandex-team.ru/WEATHER-15572
inline constexpr TStringBuf WEATHER_SUGGEST_1 = "weather_suggest_1";
inline constexpr TStringBuf WEATHER_SUGGEST_2 = "weather_suggest_2";
inline constexpr TStringBuf WEATHER_SUGGEST_3 = "weather_suggest_3";
inline constexpr TStringBuf WEATHER_SUGGEST_4 = "weather_suggest_4";
inline constexpr TStringBuf WEATHER_SUGGEST_5 = "weather_suggest_5";
inline constexpr TStringBuf WEATHER_SUGGEST_6 = "weather_suggest_6";
inline constexpr TStringBuf WEATHER_SUGGEST_7 = "weather_suggest_7";
inline constexpr TStringBuf WEATHER_SUGGEST_8 = "weather_suggest_8";
} // namespace NExperiment

} // namespace NBASS::NWeather
