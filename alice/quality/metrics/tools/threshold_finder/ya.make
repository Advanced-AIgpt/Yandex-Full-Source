PY3_PROGRAM()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/quality/metrics/lib/binary
    alice/quality/metrics/lib/core
    alice/quality/metrics/lib/multilabel
    alice/quality/metrics/lib/thresholds
    contrib/python/click
    yt/python/client
)

PY_SRCS(
    MAIN main.py
)

END()

