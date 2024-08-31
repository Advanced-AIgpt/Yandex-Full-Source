PY3_PROGRAM()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/quality/metrics/lib/thresholds
    contrib/python/click
    yt/python/client
)

PY_SRCS(
    MAIN main.py
)

END()

