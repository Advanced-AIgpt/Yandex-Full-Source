PY2_PROGRAM(yt_duplicate_filtering)

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    # 3rd party
    yt/python/client
)

PY_SRCS(
    NAMESPACE verstehen.data.yt.duplicate_filtering
    __main__.py
)

END()
