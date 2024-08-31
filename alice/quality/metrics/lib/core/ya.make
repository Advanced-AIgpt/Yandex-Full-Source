PY3_LIBRARY()

OWNER(
    sdll
    g:alice_quality
)

PY_SRCS(
    metric_type.py
    metrics.py
    utils.py
)

PEERDIR(
    contrib/python/scikit-learn
    yt/python/client
)

END()
