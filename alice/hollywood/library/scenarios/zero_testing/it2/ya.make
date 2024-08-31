PY3TEST()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/zero_testing/proto
)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

END()
