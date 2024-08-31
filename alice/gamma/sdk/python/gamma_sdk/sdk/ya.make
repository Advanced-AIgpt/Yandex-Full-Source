OWNER(
    g-kostin
    g:alice
)

PY3_LIBRARY()

PEERDIR(
    contrib/python/attrs
    alice/gamma/sdk/python/gamma_sdk/inner
)

PY_SRCS(
    NAMESPACE gamma_sdk.sdk

    __init__.py
    sdk.py
    button.py
    card.py
)

END()
