PY3TEST()

OWNER(
    yagafarov
    g:megamind
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

TEST_SRCS(
    basic_run_request.py
)

END()
