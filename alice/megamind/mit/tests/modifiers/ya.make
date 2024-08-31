PY3TEST()

OWNER(
    yagafarov
    g:megamind
)

SIZE(MEDIUM)

PEERDIR(
    alice/megamind/protos/modifiers
)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

TEST_SRCS(
    modifiers.py
)

END()
