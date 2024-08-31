PY3TEST()

OWNER(
    g:hollywood
    vitvlkv
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/alice_show/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/alice_show/it/data
)

REQUIREMENTS(ram:32)

END()
