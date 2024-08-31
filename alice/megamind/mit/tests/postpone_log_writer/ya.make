PY3TEST()

OWNER(
    yagafarov
    g:megamind
)

SIZE(MEDIUM)

PEERDIR(
    alice/megamind/library/apphost_request/protos
    alice/megamind/protos/common
)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

TEST_SRCS(
    postpone_log_writer.py
)

END()
