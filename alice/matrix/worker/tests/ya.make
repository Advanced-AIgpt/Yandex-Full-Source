PY3TEST()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/tests/library
    alice/matrix/scheduler/tests/library
    alice/matrix/worker/tests/library

    contrib/python/pytest-asyncio
)

TEST_SRCS(
    test_simple.py
    test_worker_do_send_technical_push.py
    test_worker_loop_mode.py
    test_worker_sync.py
)

INCLUDE(${ARCADIA_ROOT}/alice/matrix/scheduler/tests/library/data.ya.make.inc)
INCLUDE(${ARCADIA_ROOT}/alice/matrix/worker/tests/library/data.ya.make.inc)

INCLUDE(${ARCADIA_ROOT}/alice/matrix/library/testing/python/data.ya.make.inc)
INCLUDE(${ARCADIA_ROOT}/alice/matrix/library/testing/ydb_recipe/ya.make.inc)

SIZE(MEDIUM)
REQUIREMENTS(ram:32)

END()

RECURSE(
    library
)
