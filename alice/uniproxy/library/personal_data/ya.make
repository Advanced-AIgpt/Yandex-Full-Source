PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/uniproxy/library/auth
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings

    contrib/python/tornado/tornado-4
)

END()

RECURSE_FOR_TESTS(ut)
