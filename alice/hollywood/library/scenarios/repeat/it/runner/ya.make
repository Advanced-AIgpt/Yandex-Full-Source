PY3TEST()

OWNER(
    ikorobtsev
    g:megamind
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/repeat/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/repeat/it/data
)

REQUIREMENTS(ram:32)

END()
