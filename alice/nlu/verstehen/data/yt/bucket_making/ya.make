PY2_PROGRAM(yt_bucket_making)

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    alice/nlu/verstehen/config
    alice/nlu/verstehen/index
    # 3rd party
    contrib/python/tqdm
    yt/python/client
    contrib/python/numpy
)

PY_SRCS(
    NAMESPACE verstehen.data.yt.bucket_making
    __main__.py
    __init__.py
    bm25_data.py
    dssm_knn_data.py
    texts_data.py
    util.py
)

END()
