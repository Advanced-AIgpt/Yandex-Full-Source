PY3_LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/tfnn/preprocess
    alice/boltalka/generative/training/data/tokenizer_py
    alice/boltalka/tools/dssm_preprocessing/preprocessing/lib
    nirvana/valhalla/src
    yt/python/client
)

PY_SRCS(
    __init__.py
    lib.py
    ops.py
    preprocess.py
    util.py
    dict/__init__.py
    dict/ops.py
    experimental/__init__.py
    experimental/ops.py
    experimental/util.py
)

END()
