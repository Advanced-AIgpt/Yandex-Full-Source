OWNER(
    sdll
    g:alice_quality
)

PY3TEST()

TEST_SRCS(
    test_threshold_finder.py
    tests_metrics_calculator.py
)

PEERDIR(
    alice/quality/metrics/lib/binary
    alice/quality/metrics/lib/core
    alice/quality/metrics/lib/multiclass
    alice/quality/metrics/lib/multilabel
    alice/quality/metrics/lib/thresholds
    contrib/python/scikit-learn
)

SIZE(SMALL)

END()
