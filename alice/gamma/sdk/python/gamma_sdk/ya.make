OWNER(
    g-kostin
    g:alice
)

PY3_LIBRARY()

PEERDIR(
    contrib/libs/grpc
    contrib/python/attrs
    contrib/python/pytz
    contrib/python/ujson

    alice/gamma/sdk/api
    alice/gamma/sdk/python/gamma_sdk/inner
    alice/gamma/sdk/python/gamma_sdk/sdk
)

PY_SRCS(
    NAMESPACE gamma_sdk

    __init__.py
    client.py
    log.py
)

END()

RECURSE(
    inner
    sdk
)

RECURSE_FOR_TESTS(
    testing
    tests
)
