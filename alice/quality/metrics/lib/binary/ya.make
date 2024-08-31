PY3_LIBRARY()

OWNER(
    sdll
    g:alice_quality
)

PY_SRCS(
    binary_metrics_accumulator.py
    input_converter.py
)

PEERDIR(
    alice/quality/metrics/lib/core
    contrib/python/scikit-learn
)

END()
