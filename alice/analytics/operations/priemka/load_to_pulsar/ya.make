PY3_PROGRAM()

PEERDIR(
    mapreduce/yt/python
    contrib/python/click
    ml/pulsar/python-package-legacy

    alice/analytics/operations/dialog/pulsar
)

PY_SRCS(
    MAIN main.py
)

END()
