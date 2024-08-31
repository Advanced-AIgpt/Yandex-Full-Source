PY3TEST()

OWNER(
    ardulat
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/tr_navi/get_my_location/it
)

TEST_SRCS(runner.py)

DATA(arcadia/alice/hollywood/library/scenarios/tr_navi/get_my_location/it/data)

REQUIREMENTS(ram:32)

END()
