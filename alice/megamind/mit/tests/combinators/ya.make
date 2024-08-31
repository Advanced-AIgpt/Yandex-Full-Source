PY3TEST()

OWNER(
    nkodosov
    g:megamind
)

PEERDIR(
    alice/megamind/mit/library/common/util/combinator
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

TEST_SRCS(
    test_combinators.py
)

END()
