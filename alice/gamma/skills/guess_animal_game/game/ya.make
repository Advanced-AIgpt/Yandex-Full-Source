OWNER(
    g-kostin
    g:alice
)

PY3_LIBRARY()

PEERDIR(
    alice/gamma/sdk/python/gamma_sdk
)

PY_SRCS(
    data.py
    game.py
    resources.py
)

END()
