PY3TEST()

OWNER(
    d-dima
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

DATA(arcadia/alice/hollywood/library/scenarios/get_date/it2/tests)

END()
