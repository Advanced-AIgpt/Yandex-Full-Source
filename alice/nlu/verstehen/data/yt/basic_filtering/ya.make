PY2_PROGRAM(yt_basic_filtering)

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
    NAMESPACE verstehen.data.yt.basic_filtering
    __main__.py
)

END()
