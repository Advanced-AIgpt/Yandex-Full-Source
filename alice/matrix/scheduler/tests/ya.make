PY3TEST()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/scheduler/tests/library

    contrib/python/pytest-asyncio
)

TEST_SRCS(
    test_scheduler.py
    test_scheduler_old_http_api.py
    test_simple.py
)

INCLUDE(${ARCADIA_ROOT}/alice/matrix/scheduler/tests/library/data.ya.make.inc)

INCLUDE(${ARCADIA_ROOT}/alice/matrix/library/testing/python/data.ya.make.inc)
INCLUDE(${ARCADIA_ROOT}/alice/matrix/library/testing/ydb_recipe/ya.make.inc)

SIZE(MEDIUM)
REQUIREMENTS(ram:32)

END()

RECURSE(
    library
)
