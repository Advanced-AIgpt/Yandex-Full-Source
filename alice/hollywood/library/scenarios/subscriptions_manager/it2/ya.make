PY3TEST()

OWNER(
    jan-fazli
    g:hollywood
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/subscriptions_manager/proto
)

TEST_SRCS(
    tests_subscriptions_manager.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/subscriptions_manager/it2/tests_subscriptions_manager
)

REQUIREMENTS(ram:32)

END()
