PY3TEST()

OWNER(
    nkodosov
    g:megamind
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

END()
