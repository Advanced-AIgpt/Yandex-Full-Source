PY3_PROGRAM()

OWNER(
    sdll
    g:alice_quality
)

PY_SRCS(
    MAIN __main__.py
    
)

PEERDIR(
    alice/quality/metrics/lib/binary
    alice/quality/metrics/lib/core
    alice/quality/metrics/lib/multiclass
    alice/quality/metrics/lib/multilabel
    contrib/python/click
    contrib/python/numpy
    yt/python/client
)

END()
