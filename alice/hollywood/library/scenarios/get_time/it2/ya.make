PY3TEST()

OWNER(
    alexanderplat
    g:hollywood
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(conftest.py tests.py)

REQUIREMENTS(ram:32)

PEERDIR(
    alice/hollywood/library/scenarios/get_time/proto
)

DATA(arcadia/alice/hollywood/library/scenarios/get_time/it2/tests)

END()
