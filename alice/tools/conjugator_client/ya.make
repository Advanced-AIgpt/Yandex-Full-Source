PY3_PROGRAM()

OWNER(alexanderplat)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/protos/api/conjugator
    apphost/python/client
    yt/python/client
)

END()
