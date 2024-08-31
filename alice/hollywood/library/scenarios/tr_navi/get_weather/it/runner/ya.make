PY3TEST()

OWNER(
    flimsywhimsy
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/tr_navi/get_weather/it
)

TEST_SRCS(runner.py)

DATA(arcadia/alice/hollywood/library/scenarios/tr_navi/get_weather/it/data)

REQUIREMENTS(ram:32)

END()
