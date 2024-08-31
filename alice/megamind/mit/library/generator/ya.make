PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/library/python/eventlog_wrapper
    alice/megamind/mit/library/graphs_util
    alice/megamind/mit/library/request_builder
    alice/megamind/mit/library/requester
    alice/megamind/mit/library/response
)

PY_SRCS(
    __init__.py
    mit_generator.py
)

END()
