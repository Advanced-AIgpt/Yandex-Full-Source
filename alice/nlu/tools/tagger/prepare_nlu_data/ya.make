OWNER(
    the0
    g:alice_quality
)

PY2_PROGRAM()

PEERDIR(
    alice/nlu/py_libs/request_normalizer
    alice/nlu/py_libs/utils
    contrib/python/tqdm
    yt/python/client
)

PY_SRCS(
    MAIN prepare_nlu_data.py
)

END()
