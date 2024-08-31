PY3TEST()

OWNER(
    defolter
    g:yandex_drive_auto
)
SIZE(medium)
DEPENDS(${ARCADIA_ROOT}/alice/kronstadt/scenarios/automotive_hvac/it2/shard)

INCLUDE(${ARCADIA_ROOT}/alice/kronstadt/it2/common.inc)
NO_CHECK_IMPORTS()

TEST_SRCS(
    test.py
)

END()

RECURSE(shard)
