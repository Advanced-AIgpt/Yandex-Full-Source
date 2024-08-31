PY3TEST()

OWNER(
    yagafarov
    g:megamind
)

PEERDIR(
    alice/megamind/library/stack_engine/protos
    alice/megamind/protos/scenarios
    contrib/python/protobuf
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

TEST_SRCS(
    warm_up.py
)

END()
