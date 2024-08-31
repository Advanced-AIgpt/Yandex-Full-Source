PY3TEST()

OWNER(
    alexanderplat
    g:megamind
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

PEERDIR(
    apphost/lib/proto_answers
)

TEST_SRCS(
    lib.py
    polyglot_begemot.py
    polyglot_pipeline.py
)

END()
