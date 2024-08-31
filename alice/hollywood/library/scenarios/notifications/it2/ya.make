PY3TEST()

OWNER(
    redin-d
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

FORK_SUBTESTS()

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    tests_animation.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/notifications/it2/tests_animation
)

END()
