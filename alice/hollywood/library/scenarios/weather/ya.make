LIBRARY()

OWNER(
    sparkle
    g:alice
)

FROM_SANDBOX(FILE 2890819033 AUTOUPDATED translations OUT_NOAUTO translations.json)

FROM_SANDBOX(FILE 1223215759 OUT_NOAUTO avatars.json)

RESOURCE(
    translations.json translations.json
)

RESOURCE(
    avatars.json avatars.json
)

PEERDIR(
    alice/hollywood/library/capability_wrapper
    alice/hollywood/library/framework
    alice/hollywood/library/registry
    alice/hollywood/library/s3_animations
    alice/hollywood/library/scenarios/weather/background_sounds
    alice/hollywood/library/scenarios/weather/nlg
    alice/hollywood/library/scenarios/weather/proto
    alice/hollywood/library/scenarios/weather/s3_animations
    alice/library/datetime
    alice/library/geo
    alice/library/geo_resolver
    alice/library/logger
    alice/library/scled_animations
    alice/library/util
    alice/library/versioning
    apphost/lib/proto_answers
    search/idl
    search/session/compression
    weather/app_host/accumulated_precipitations/protos
    weather/app_host/fact/protos
    weather/app_host/forecast_postproc/lib
    weather/app_host/forecast_postproc/protos
    weather/app_host/geo_location/lib
    weather/app_host/geo_location/protos
    weather/app_host/magnetic_field/protos
    weather/app_host/meteum/protos
    weather/app_host/nowcast/protos
    weather/app_host/protos
    weather/app_host/v3_nowcast_alert_response/protos
    weather/app_host/warnings/protos
)

JOIN_SRCS_GLOBAL(
    all.cpp
    register.cpp
    cases/change.cpp
    cases/nowcast/nowcast_by_hours_weather.cpp
    cases/nowcast/nowcast_day_part_weather.cpp
    cases/nowcast/nowcast_default_weather.cpp
    cases/nowcast/nowcast_for_now_weather.cpp
    cases/nowcast/nowcast_util.cpp
    cases/nowcast/switch_weather.cpp
    cases/pressure_cases.cpp
    cases/weather/current_weather.cpp
    cases/weather/day_hours_weather.cpp
    cases/weather/day_part_weather.cpp
    cases/weather/day_weather.cpp
    cases/weather/days_range_weather.cpp
    cases/weather/today_weather.cpp
    cases/wind/current_wind.cpp
    cases/wind/day_part_wind.cpp
    cases/wind/day_wind.cpp
    cases/wind/days_range_wind.cpp
    cases/wind/today_wind.cpp
    context/api.cpp
    context/context.cpp
    context/renderer.cpp
    context/weather_protos.cpp
    handles/parse_geometasearch_handle.cpp
    handles/parse_reqwizard_handle.cpp
    handles/prepare_city_handle.cpp
    handles/prepare_common_handle.cpp
    handles/prepare_forecast_handle.cpp
    handles/render_handle.cpp
    request_helper/geometasearch.cpp
    request_helper/reqwizard.cpp
    util/avatars.cpp
    util/translations.cpp
    util/util.cpp
)

GENERATE_ENUM_SERIALIZATION(util/error.h)

END()

RECURSE_FOR_TESTS(
    background_sounds/ut
    it2
    s3_animations/ut
    ut
)
