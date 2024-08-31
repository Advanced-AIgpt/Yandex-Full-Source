PY3TEST()

OWNER(
    caesium
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

PEERDIR(
    alice/hollywood/library/scenarios/order/proto
)

DATA(arcadia/alice/hollywood/library/scenarios/order/it2/tests)

END()