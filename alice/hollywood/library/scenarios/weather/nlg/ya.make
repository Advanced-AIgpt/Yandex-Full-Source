LIBRARY()

OWNER(
    sparkle
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/common_nlg
)

COMPILE_NLG(
    cards/weather.nlg
    cards/weather_nowcast.nlg
    cards/weather_carousel.nlg
    cards/weather_show_view.nlg
    errors_ar.nlg
    errors_ru.nlg
    feedback_ar.nlg
    feedback_ru.nlg
    get_weather__ask_ar.nlg
    get_weather__ask_ru.nlg
    get_weather__common_ar.nlg
    get_weather__common_ru.nlg
    get_weather__details_ar.nlg
    get_weather__details_ru.nlg
    get_weather__today_ar.nlg
    get_weather__today_ru.nlg
    get_weather_change_ru.nlg
    get_weather_nowcast_ar.nlg
    get_weather_nowcast_ru.nlg
    get_weather_nowcast_text_cases_ar.nlg
    get_weather_nowcast_text_cases_ru.nlg
    get_weather_ar.nlg
    get_weather_ru.nlg
    get_weather_pressure_ar.nlg
    get_weather_pressure_ru.nlg
    get_weather_wind_ar.nlg
    get_weather_wind_ru.nlg
    irrelevant_ar.nlg
    irrelevant_ru.nlg
    suggests_ar.nlg
    suggests_ru.nlg
)

END()
