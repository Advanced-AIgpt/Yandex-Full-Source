PY3_LIBRARY()

OWNER(
    sdll
    g:alice_quality
)

PY_SRCS(
    input_converter.py
    multilabel_metrics_accumulator.py
)

PEERDIR(
    alice/quality/metrics/lib/core
)

END()
