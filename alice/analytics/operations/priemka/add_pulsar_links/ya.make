PY3_PROGRAM()

PEERDIR(
    contrib/python/click
    ml/pulsar/python-package-legacy

    alice/analytics/operations/dialog/pulsar
)

PY_SRCS(
    MAIN main.py
)

END()
