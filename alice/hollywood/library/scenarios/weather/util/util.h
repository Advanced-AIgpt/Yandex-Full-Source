#pragma once

#include <alice/hollywood/library/scenarios/weather/background_sounds/background_sounds.h>
#include <alice/hollywood/library/scenarios/weather/context/context.h>
#include <alice/hollywood/library/scenarios/weather/fwd.h>
#include <alice/hollywood/library/scenarios/weather/proto/weather.pb.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/request/experiments.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/scenario/weather/weather.pb.h>

#include <alice/library/datetime/datetime.h>
#include <alice/library/logger/logger.h>
#include <alice/library/client/client_features.h>

#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NHollywood::NWeather {

namespace NFrameNames {

constexpr TStringBuf COLLECT_CENTAUR_CARDS = "alice.centaur.collect_cards";
constexpr TStringBuf COLLECT_CENTAUR_MAIN_SCREEN = "alice.centaur.collect_main_screen";
constexpr TStringBuf COLLECT_CENTAUR_WIDGET_GALLERY = "alice.centaur.collect_widget_gallery";
constexpr TStringBuf COLLECT_CENTAUR_TEASERS_PREVIEW = "alice.centaur.collect_teasers_preview";
constexpr TStringBuf GET_WEATHER = "personal_assistant.scenarios.get_weather";
constexpr TStringBuf GET_WEATHER__ELLIPSIS = "personal_assistant.scenarios.get_weather__ellipsis";
constexpr TStringBuf GET_WEATHER__DETAILS = "personal_assistant.scenarios.get_weather__details";
constexpr TStringBuf GET_WEATHER_CHANGE = "alice.get_weather.change";
constexpr TStringBuf GET_WEATHER_NOWCAST = "personal_assistant.scenarios.get_weather_nowcast";
constexpr TStringBuf GET_WEATHER_NOWCAST__ELLIPSIS = "personal_assistant.scenarios.get_weather_nowcast__ellipsis";
constexpr TStringBuf GET_WEATHER_NOWCAST_PREC_MAP = "alice.scenarios.get_weather_nowcast_prec_map";
constexpr TStringBuf GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS = "alice.scenarios.get_weather_nowcast_prec_map__ellipsis";
constexpr TStringBuf GET_WEATHER_PRESSURE = "alice.scenarios.get_weather_pressure";
constexpr TStringBuf GET_WEATHER_PRESSURE__ELLIPSIS = "alice.scenarios.get_weather_pressure__ellipsis";
constexpr TStringBuf GET_WEATHER_WIND = "alice.scenarios.get_weather_wind";
constexpr TStringBuf GET_WEATHER_WIND__ELLIPSIS = "alice.scenarios.get_weather_wind__ellipsis";

} // namespace NFrameNames

namespace NNlgTemplateNames {

constexpr TStringBuf DEFAULT = "get_weather";
constexpr TStringBuf GET_WEATHER = "get_weather";
constexpr TStringBuf GET_WEATHER_CHANGE = "get_weather_change";
constexpr TStringBuf GET_WEATHER_DETAILS = "get_weather__details";
constexpr TStringBuf GET_WEATHER_NOWCAST = "get_weather_nowcast";
constexpr TStringBuf GET_WEATHER_PRESSURE = "get_weather_pressure";
constexpr TStringBuf GET_WEATHER_WIND = "get_weather_wind";

constexpr TStringBuf ERRORS = "errors";
constexpr TStringBuf FEEDBACK = "feedback";
constexpr TStringBuf IRRELEVANT = "irrelevant";
constexpr TStringBuf SUGGESTS = "suggests";

} // namespace NNlgTemplateNames

namespace NFeedbackOptions {

const TString POSITIVE = "feedback_positive_weather";
const TString NEGATIVE = "feedback_negative_weather";

} // namespace NFeedbackOptions

namespace NFeedbackOptionsReasons {

constexpr TStringBuf BAD_ANSWER = "feedback_negative__bad_answer";
constexpr TStringBuf ASR_ERROR = "feedback_negative__asr_error";
constexpr TStringBuf TTS_ERROR = "feedback_negative__tts_error";
constexpr TStringBuf OFFENSIVE_ANSWER = "feedback_negative__offensive_answer";
constexpr TStringBuf OTHER = "feedback_negative__other";
constexpr TStringBuf ALL_GOOD = "feedback_negative__all_good";

} // namespace NFeedbackOptions

namespace NExperiment {

// New NLG for weather today - https://st.yandex-team.ru/DIALOG-5039
constexpr TStringBuf DISABLE_NEW_NLG = "disable_new_nlg"; // Disables the new NLG without comparing to yesterday
constexpr TStringBuf NEW_NLG_COMPARE = "new_nlg_compare"; // Enables the new NLG with comparing to yesterday

// Experiments with suggests — https://st.yandex-team.ru/WEATHER-15572
constexpr TStringBuf WEATHER_SUGGEST_1 = "weather_suggest_1";
constexpr TStringBuf WEATHER_SUGGEST_2 = "weather_suggest_2";
constexpr TStringBuf WEATHER_SUGGEST_3 = "weather_suggest_3";
constexpr TStringBuf WEATHER_SUGGEST_4 = "weather_suggest_4";
constexpr TStringBuf WEATHER_SUGGEST_5 = "weather_suggest_5";
constexpr TStringBuf WEATHER_SUGGEST_6 = "weather_suggest_6";
constexpr TStringBuf WEATHER_SUGGEST_7 = "weather_suggest_7";
constexpr TStringBuf WEATHER_SUGGEST_8 = "weather_suggest_8";

// Experiments with nowcasts map — https://st.yandex-team.ru/WEATHER-18008
constexpr TStringBuf WEATHER_OPEN_NOWCAST_IN_BROWSER = "weather_open_nowcast_in_browser";

// Experiments with nowcasts map — https://st.yandex-team.ru/WEATHER-18060
constexpr TStringBuf WEATHER_OPEN_NOWCAST_IN_SUGGEST = "weather_open_nowcast_in_suggest";

// WEATHER-18052: В "дождевом" сценарии Алисы говорить про осадки на сутки
constexpr TStringBuf WEATHER_SEARCH_PRECS_IN_HOURS = "weather_search_precs_in_hours";

// WEATHER-18123: Эксперимент с заменой сводки по погоде на сегодня в Алисе на предупреждение
constexpr TStringBuf WEATHER_TODAY_FORECAST_WARNING = "weather_today_forecast_warning";

// WEATHER-18275: Эксперимент в Алисе с дополнением сводки по погоде на сейчас предупреждениями
constexpr TStringBuf WEATHER_NOW_FORECAST_WARNING = "weather_now_forecast_warning";
constexpr TStringBuf WEATHER_NOW_SIGNIFICANCE_THRESHOLD = "weather_now_significance_threshold=";
constexpr TStringBuf WEATHER_NOW_ONLY_SIGNIFICANT = "weather_now_only_significant";

// ALICEPRODUCT-371: Эксперимент с заменой сводки по погоде на выходные, эту и следующую недели в Алисе на предупреждение
constexpr TStringBuf WEATHER_FOR_RANGE_FORECAST_WARNING = "weather_for_range_forecast_warning";

// ALICEPRODUCT-370: Эксперимент с новой логикой для запросов об изменении погоды
constexpr TStringBuf WEATHER_CHANGE = "hw_weather_change";

// Experiments for Cloud UI — https://st.yandex-team.ru/DIALOG-7685
constexpr TStringBuf WEATHER_USE_CLOUD_UI = "weather_use_cloud_ui";

// Experiment for using wind precautions - https://st.yandex-team.ru/WEATHER-18130
constexpr TStringBuf WEATHER_USE_WIND_PRECAUTIONS = "weather_use_wind_precautions";

// Exp flag for saving context beetwen different weather sub-scenarios - https://st.yandex-team.ru/WEATHER-18389
constexpr TStringBuf WEATHER_USE_CROSS_SCENARIO_ELLIPSIS = "weather_use_cross_scenario_ellipsis";

// Exp flag for enabling background sounds
constexpr TStringBuf WEATHER_ENABLE_BACKGROUND_SOUNDS = "weather_enable_background_sounds";

// Exp flag for disabling days forecast for current weather - https://st.yandex-team.ru/WEATHER-18633
constexpr TStringBuf WEATHER_DISABLE_DAYS_IN_CURRENT_WEATHER = "weather_disable_days_in_current";

// Exp flag for always show days forecast for current weather - https://st.yandex-team.ru/WEATHER-18633
constexpr TStringBuf WEATHER_ALWAYS_DAYS_IN_CURRENT_WEATHER = "weather_always_days_in_current";

// Exp flag for disabling GeoMetaSearch - https://st.yandex-team.ru/HOLLYWOOD-692
constexpr TStringBuf WEATHER_DISABLE_GEO_META_SEARCH = "weather_disable_geo_meta_search";

// Exp flag for disabling requesting ru-only phrases from weather backend - https://st.yandex-team.ru/HOLLYWOOD-764#620e337fd0ba755998e6b930
constexpr TStringBuf WEATHER_DISABLE_RU_ONLY_PHRASES = "weather_disable_ru_only_phrases";

// Exp flag for new centaur main screen widget mechanics - https://st.yandex-team.ru/CENTAUR-812, https://st.yandex-team.ru/CENTAUR-688
constexpr TStringBuf SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME = "scenario_widget_mechanics";

// Exp flag for centaur typed actions - https://st.yandex-team.ru/CENTAUR-1128
constexpr TStringBuf CENTAUR_TYPED_ACTION_EXP_FLAG_NAME = "centaur_typed_action";

} // namespace NExperiment

constexpr TStringBuf YANDEX_POGODA_URL = "https://yandex.ru/pogoda";

const TString CALLBACK_FEEDBACK_NAME = "alice.weather_feedback";
const TString CALLBACK_UPDATE_FORM_NAME = "update_form";

// from most wanted frame to less wanted frame
const TSmallVec<TStringBuf> WEATHER_FRAMES_ORDER = {
    NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS,
    NFrameNames::GET_WEATHER_PRESSURE,
    NFrameNames::GET_WEATHER_WIND__ELLIPSIS,
    NFrameNames::GET_WEATHER_WIND,
    NFrameNames::GET_WEATHER_CHANGE,
    NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS,
    NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP,
    NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS,
    NFrameNames::GET_WEATHER_NOWCAST,
    NFrameNames::GET_WEATHER__DETAILS,
    NFrameNames::GET_WEATHER__ELLIPSIS,
    NFrameNames::GET_WEATHER,
    NFrameNames::COLLECT_CENTAUR_CARDS,
    NFrameNames::COLLECT_CENTAUR_TEASERS_PREVIEW,
    NFrameNames::COLLECT_CENTAUR_MAIN_SCREEN,
    NFrameNames::COLLECT_CENTAUR_WIDGET_GALLERY
};

TMaybe<TString> GetFrameNameFromCallback(const NScenarios::TCallbackDirective& callback);
bool IsNewFrameLegal(const TStringBuf prevFrameName, const TStringBuf newFrameName);
bool IsTakeSlotsFromPrevFrame(const TStringBuf newFrameName, const TExpFlags& expFlags = {});
bool IsGeoMetaSearchDisabled(const TExpFlags& expFlags = {});
void FillFromPrevFrame(TRTLogger& logger, const TFrame& prevFrame, TFrame& frame);
void FixDayPartSlot(TRTLogger& logger, const TPtrWrapper<TSlot>& slot);

TStringBuf GetFrameName(const TWeatherState& state);
TMaybe<TString> GetWeatherForecastUri(const TFrame& frame);
TString ConstructSlotRememberValue(const NJson::TJsonValue& value);

bool IsWeatherChangeScenario(const TStringBuf frameName);
bool IsNowcastWeatherScenario(const TStringBuf frameName);
bool IsPrecMapWeatherScenario(const TStringBuf frameName);
bool IsPressureWeatherScenario(const TStringBuf frameName);
bool IsWeatherScenario(const TStringBuf frameName);
bool IsWindWeatherScenario(const TStringBuf frameName);

TString TranslateWindDirection(const TString& direction, const ELanguage language, TRTLogger& logger = TRTLogger::NullLogger());

class TDay;
int GetPressureMMFromDay(const TDay& day);
TString GetTranslatedWindDirectionFromDay(const TDay& day, const ELanguage language, TRTLogger& logger = TRTLogger::NullLogger());
double GetWindSpeedFromDay(const TDay& day);
double GetWindGustFromDay(const TDay& day);

/*
 * Functions ported from BASS
 */
bool IsSlotEmpty(const TPtrWrapper<TSlot>& slot);

TWeatherErrorOr<std::unique_ptr<NAlice::TDateTimeList>> GetDateTimeList(const TWeatherContext& ctx, const TDateTime::TSplitTime& userTime);

NJson::TJsonValue GetHours(const THour& hour);

TWeatherErrorOr<NJson::TJsonValue> GetForecastLocationSlot(const TWeatherContext& ctx);
TWeatherErrorOr<NJson::TJsonValue> GetOriginalForecastLocationSlot(const TWeatherContext& ctx);

TWeatherErrorOr<TString> GetWeatherUrl(const TWeatherContext& ctx, TMaybe<int> anchorDay = Nothing());
TWeatherErrorOr<TString> GetWeatherWindUrl(const TWeatherContext& ctx, TMaybe<int> anchorDay = Nothing());
TWeatherErrorOr<TString> GetWeatherMonthUrl(const TWeatherContext& ctx, TMaybe<int> anchorDay = Nothing());

TMaybe<TWeatherError> AddForecastLocationSlots(TWeatherContext& ctx);

std::pair<int, int> GetMinMaxTempsFromDayParts(const TParts& parts);

const TVector<NScenarios::TLayout::TButton>& NoButtons();

void FillShowViewHourItem(::NAlice::NData::TWeatherHourItem& hourItem, const THour& hour);

void FillShowViewGeoLocation(::NAlice::NData::TWeatherLocation& location, const TWeatherContext& ctx);

NDatetime::TCivilSecond ConvertTCivilTime(const TWeatherProtos::TCivilTime& time);

TString ConstructScledPattern(const int temperature);

void TryAddWeatherGeoId(TRTLogger& logger, NAppHost::IServiceContext& serviceCtx, TWeatherErrorOr<NGeobase::TId> parsedGeoId);

class TFact;
void FillBackgroundSoundsFromFact(TBackgroundSounds& backgroundSounds, const TFact& fact);

bool IsElementNull(const NJson::TJsonValue& value, const TStringBuf key);

void FixWhenSlotForNextWeekend(TWeatherContext& ctx);

} // namespace NAlice::NHollywood::NWeather
