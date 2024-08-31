PY3_PROGRAM()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/tools/dssm_preprocessing/preprocessing/lib
    alice/boltalka/generative/training/data/nn/util

    yt/python/client
    nirvana/valhalla/src

    bindings/python/lemmer_lib
)

PY_SRCS(
    __init__.py
    __main__.py
)

END()
