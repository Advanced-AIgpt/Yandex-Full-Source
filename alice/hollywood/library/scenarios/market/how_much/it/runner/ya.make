PY3TEST()

OWNER(
    g:marketinalice
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/market/how_much/it
)

TEST_SRCS(
    test.py
)

SIZE(MEDIUM)

DATA(
    arcadia/alice/hollywood/library/scenarios/market/how_much/it/data
)

REQUIREMENTS(ram:32)

END()
