PY3TEST()

OWNER(
    g:paskills
)
SIZE(medium)
DEPENDS(${ARCADIA_ROOT}/alice/kronstadt/scenarios/photoframe/it2/shard)

INCLUDE(${ARCADIA_ROOT}/alice/kronstadt/it2/common.inc)
NO_CHECK_IMPORTS()

TEST_SRCS(
    test.py
)

END()

RECURSE(shard)

