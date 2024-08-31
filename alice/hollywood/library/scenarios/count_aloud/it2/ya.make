PY3TEST()

OWNER(
    olegator
    g:alice_quality
)

SIZE(MEDIUM)

FORK_SUBTESTS()

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

END()
