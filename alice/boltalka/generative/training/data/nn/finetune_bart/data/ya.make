PY3_PROGRAM()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/training/data/nn/util
    alice/boltalka/generative/tfnn/preprocess

    yt/python/client
    nirvana/valhalla/src
)

PY_SRCS(
    __init__.py
    __main__.py
    data.py
)

END()
