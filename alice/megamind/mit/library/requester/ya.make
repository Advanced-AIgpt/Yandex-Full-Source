PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/megamind/mit/library/request_builder
    alice/tests/library/service
)

PY_SRCS(
    __init__.py
    requester.py
)

END()
