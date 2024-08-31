PY3_PROGRAM()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/training/data/nn/util

    yt/python/client
    nirvana/valhalla/src

    ml/libs/ml_data_reader/src
    ml/tensorflow/tfnn/src
)

PY_SRCS(
    __init__.py
    __main__.py
    ops.py
)

END()
