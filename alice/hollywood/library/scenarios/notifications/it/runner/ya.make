PY3TEST()

OWNER(
    tolyandex
    g:hollywood
    g:alice
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/notifications/it
)

TEST_SRCS(tests.py)

SIZE(MEDIUM)

DATA(arcadia/alice/hollywood/library/scenarios/notifications/it/data)

REQUIREMENTS(ram:32)

END()
