PY2_PROGRAM()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    __main__.py
    TOP_LEVEL    alice/boltalka/scripts/twitter_dataset_build/watson/constants.py
)

PEERDIR(
    yt/python/client
)

END()
