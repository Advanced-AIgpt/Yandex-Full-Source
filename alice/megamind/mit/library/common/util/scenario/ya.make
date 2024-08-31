PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/megamind/mit/library/common/names
    alice/megamind/mit/library/util
    alice/megamind/protos/scenarios
)

PY_SRCS(
    responses.py
)

END()
