PY3TEST()

OWNER(
    jan-fazli
    g:hollywood
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    tests_onboarding.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/subscriptions_manager/it2/tests_onboarding
)

REQUIREMENTS(ram:32)

END()
