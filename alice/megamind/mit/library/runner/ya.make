PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/megamind/mit/library/request_builder
    alice/megamind/mit/library/requester
    alice/megamind/mit/library/response
)

PY_SRCS(
    __init__.py
    mit_runner.py
)

END()
