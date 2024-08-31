PY3TEST()

OWNER(
    g:maps-mobile-alice
)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    conftest.py
    tests.py
)

REQUIREMENTS(ram:32)

END()
