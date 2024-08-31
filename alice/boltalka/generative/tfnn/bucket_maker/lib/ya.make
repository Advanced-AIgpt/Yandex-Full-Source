PY3_LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/tfnn/infer_lib
    alice/boltalka/generative/tfnn/preprocess
    alice/boltalka/generative/training/data/tokenizer_py

    yt/python/client
)

PY_SRCS(
    __init__.py
    bucket.py
)

END()
