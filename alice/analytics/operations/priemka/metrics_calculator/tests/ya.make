OWNER(g:alice_analytics)

PY3TEST()

PEERDIR(
    alice/analytics/operations/priemka/metrics_calculator
    alice/analytics/operations/priemka/alice_parser/utils
    alice/analytics/tasks/VA-571
    alice/analytics/utils/auth
    alice/analytics/utils/yt

    alice/analytics/utils/testing_utils
    contrib/python/pytest
    statbox/nile
    statbox/nile_debug
    statbox/qb2
    yql/library/python

)

TEST_SRCS(
    test_calc_metrics.py
    test_asr_metrics.py
    test_nlg_metrics.py
    test_calc_metrics_yt.py
    test_metrics_calculator.py
    test_metric.py
    test_utils.py
    test_call_metric.py
)

NO_CHECK_IMPORTS()

END()
