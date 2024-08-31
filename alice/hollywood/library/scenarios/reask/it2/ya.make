PY3TEST()

OWNER(
    ardulat
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/reask/proto
)

TEST_SRCS(tests.py)

DATA(arcadia/alice/hollywood/library/scenarios/reask/it2/tests)

REQUIREMENTS(ram:32)

END()
