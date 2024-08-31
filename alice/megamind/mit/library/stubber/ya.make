PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/library/python/eventlog_wrapper
    alice/megamind/mit/library/graphs_util
    alice/megamind/mit/library/util
    apphost/api/service/python
    apphost/lib/proto_answers
    apphost/python/client
    contrib/python/falcon
    contrib/python/requests
    contrib/python/retry
    devtools/ya/yalibrary/upload
    library/python/testing/yatest_common
)

PY_SRCS(
    __init__.py
    apphost_stubber.py
    http_proxy.py
    stubber_repository.py
)

END()
