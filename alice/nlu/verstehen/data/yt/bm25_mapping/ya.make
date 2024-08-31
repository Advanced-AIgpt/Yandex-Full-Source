PY2_PROGRAM(yt_bm25_mapping)

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    # 3rd party
    yt/python/client
    alice/nlu/verstehen/preprocess
)

PY_SRCS(
    NAMESPACE verstehen.data.yt.bm25_mapping
    __main__.py
)

END()
