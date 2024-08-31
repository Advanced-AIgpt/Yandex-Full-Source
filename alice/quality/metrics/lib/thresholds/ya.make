PY3_LIBRARY()

OWNER(
    vl-trifonov
    g:alice_quality
)

PY_SRCS(
    plots.py
    threshold_applier.py
    threshold_finder.py
)

PEERDIR(
    alice/quality/metrics/lib/binary
    alice/quality/metrics/lib/core
    contrib/python/numpy
    contrib/python/plotly
    contrib/python/scikit-learn
)

END()
