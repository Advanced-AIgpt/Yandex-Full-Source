PY3TEST()

OWNER(
    g:paskills
)
SIZE(medium)
DEPENDS(${ARCADIA_ROOT}/alice/kronstadt/scenarios/contacts/it2/shard)

INCLUDE(${ARCADIA_ROOT}/alice/kronstadt/it2/common.inc)
NO_CHECK_IMPORTS()

PY_SRCS(
    contact_updates.py
)

TEST_SRCS(
    test.py
)

END()

RECURSE(shard)
