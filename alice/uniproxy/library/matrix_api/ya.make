PY3_LIBRARY()

OWNER(
    g:matrix
)

PY_SRCS(
    __init__.py
    matrix_api.py
)

PEERDIR(
    alice/protos/api/matrix

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/logging
    alice/uniproxy/library/notificator_api
    alice/uniproxy/library/settings

    alice/rtlog/client/python/lib

    contrib/python/tornado/tornado-4
)

END()

RECURSE_FOR_TESTS(ut)
