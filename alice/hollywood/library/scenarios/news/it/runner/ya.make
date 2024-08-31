PY3TEST()

OWNER(
    g:hollywood
    khr2
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/news/it
    alice/hollywood/library/scenarios/news/proto
)

TEST_SRCS(
    tests_push.py
    tests.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/news/it/data_push
    arcadia/alice/hollywood/library/scenarios/news/it/data
)

REQUIREMENTS(ram:32)

END()
