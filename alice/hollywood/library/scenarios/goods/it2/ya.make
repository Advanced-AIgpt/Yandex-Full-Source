PY3TEST()

OWNER(
    vkaneva
    g:goods-runtime
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/goods/proto
)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

END()
