PY3TEST()

OWNER(
    sparkle
)

IF(CLANG_COVERAGE)
    # 1.5 hours timeout
    SIZE(LARGE)
    TAG(ya:fat ya:force_sandbox ya:sandbox_coverage)
ELSE()
    # 15 minutes timeout
    SIZE(MEDIUM)
ENDIF()

FORK_SUBTESTS()
SPLIT_FACTOR(4)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/weather/proto
    alice/megamind/protos/scenarios
    contrib/python/PyHamcrest
    weather/app_host/forecast_postproc/protos
    weather/app_host/geo_location/protos
    weather/app_host/protos
    weather/app_host/v3_nowcast_alert_response/protos
    weather/app_host/warnings/protos
)

TEST_SRCS(
    conftest.py
    tests_animations.py
    tests_background_sounds.py
    tests_change.py
    tests_experiments_tts.py
    tests_feedback.py
    tests_led.py
    tests_now_voice.py
    tests_nowcast.py
    tests_prec_map.py
    tests_pressure.py
    tests_region.py
    tests_scenario_state.py
    tests_semantic_frame.py
    tests_smart_display.py
    tests_smoke.py
    tests_today_voice.py
    tests_tomorrow_voice.py
    tests_wind.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_experiments_tts
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_led
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_now_voice
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_nowcast
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_prec_map
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_pressure
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_semantic_frame
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_smart_display
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_smoke
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_wind
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_today_voice
    arcadia/alice/hollywood/library/scenarios/weather/it2/tests_tomorrow_voice
    arcadia/alice/megamind/protos/common
    arcadia/alice/megamind/protos/scenarios
    arcadia/alice/protos/api/renderer
)

REQUIREMENTS(ram:32)

END()
