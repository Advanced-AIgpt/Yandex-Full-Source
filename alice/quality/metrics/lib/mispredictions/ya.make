PY3_LIBRARY()

OWNER(
    vl-trifonov
    g:alice_quality
)

PY_SRCS(
    misprediction_finder.py
)

PEERDIR(
    alice/quality/metrics/lib/binary
    alice/quality/metrics/lib/core
    alice/quality/metrics/lib/multilabel
    yt/python/client
)

END()
