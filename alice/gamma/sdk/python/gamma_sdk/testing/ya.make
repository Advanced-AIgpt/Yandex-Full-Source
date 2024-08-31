OWNER(
    g-kostin
    g:alice
)

PY3_LIBRARY()

PEERDIR(
    contrib/python/attrs
    alice/gamma/sdk/python/gamma_sdk/inner
    alice/gamma/sdk/python/gamma_sdk/sdk
)

PY_SRCS(
    NAMESPACE gamma_sdk.testing

    __init__.py
    sdk.py
)

END()
