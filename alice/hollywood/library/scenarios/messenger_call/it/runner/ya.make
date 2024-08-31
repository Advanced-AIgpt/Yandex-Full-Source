PY3TEST()

OWNER(
    akastornov
    g:hollywood
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/messenger_call/it
)

TEST_SRCS(
    runner.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/messenger_call/it/data
)

REQUIREMENTS(ram:32)

END()
