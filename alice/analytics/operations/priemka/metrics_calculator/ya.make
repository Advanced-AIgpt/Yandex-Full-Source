OWNER(g:alice_analytics)

PY3_LIBRARY()

PEERDIR(
    contrib/python/scipy
    alice/analytics/tasks/VA-571

    voicetech/asr/tools/metrics/lib
    quality/ab_testing/cofe/projects/alice
)

PY_SRCS(
    calc_metrics_local.py
    calc_metrics_yt.py
    metric.py
    utils.py
    markdown_patterns.py
    metrics_calculator.py

    metrics/__init__.py
    metrics/diff_metrics.py
    metrics/quality_metrics.py
    metrics/classification_metrics.py
    metrics/error_metrics.py
    metrics/toloka_stats.py
    metrics/asr_metrics.py
    metrics/nlg_metrics.py
    metrics/utils.py
    metrics/call_metrics.py
)

NO_CHECK_IMPORTS()

END()

RECURSE(
    bin
    yt
    udf
)

RECURSE_FOR_TESTS(
    tests
)
