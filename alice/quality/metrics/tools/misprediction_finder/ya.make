PY3_PROGRAM()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/quality/metrics/lib/core
    alice/quality/metrics/lib/mispredictions
    contrib/python/click
    yt/python/client
)

PY_SRCS(
    MAIN main.py
)

END()

