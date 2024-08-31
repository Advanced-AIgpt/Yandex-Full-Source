PY3TEST()

OWNER(
    g:alice-time-capsule-scenario
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/time_capsule/proto
)

TEST_SRCS(tests.py)

DATA(arcadia/alice/hollywood/library/scenarios/time_capsule/it2/tests)

END()
