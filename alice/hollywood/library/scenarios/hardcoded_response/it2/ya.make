PY3TEST()

OWNER(
    yagafarov
    g:megamind
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    tests_hardcoded_response.py
)

DATA(
    arcadia/alice/hollywood/library/hardcoded_response/it2/tests_hardcoded_response
)

REQUIREMENTS(ram:10)

END()
