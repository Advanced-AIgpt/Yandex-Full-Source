PY3TEST()

OWNER(alexanderplat g:hollywood)

TEST_SRCS(
    test_scenario_nlg.py
)

PEERDIR(
    alice/nlg/library/python/nlg_renderer

    alice/hollywood/library/scenarios/weather/nlg
)

DATA(
    arcadia/alice/hollywood/tests/nlg_tests/data/weather_ar.json
)

END()
