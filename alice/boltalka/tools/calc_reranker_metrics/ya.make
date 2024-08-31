PY2_PROGRAM()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    __main__.py
    utils.py
)

PEERDIR(
    yt/python/client
    contrib/python/numpy
)

END()
