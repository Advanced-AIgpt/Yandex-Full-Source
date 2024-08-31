PY3TEST()

OWNER(
    nkodosov
    g:megamind
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

TEST_SRCS(
    test_conditional_datasources.py
)

END()
