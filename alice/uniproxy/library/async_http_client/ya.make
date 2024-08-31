PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    http_client.py
    http_connection.py
    http_request.py
    rtlog_http_client.py
    rtlog_http_request.py
    settings.py
)

PEERDIR(
    alice/rtlog/client/python/lib
    alice/uniproxy/library/logging
    alice/uniproxy/library/utils

    contrib/python/tornado/tornado-4
)

END()

RECURSE_FOR_TESTS(ut)
