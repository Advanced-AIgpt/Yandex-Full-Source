PY3_PROGRAM()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    yt/python/client
    nirvana/valhalla/src
    alice/boltalka/valhalla
)

END()
