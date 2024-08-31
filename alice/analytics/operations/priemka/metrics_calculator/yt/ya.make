OWNER(g:alice_analytics)

PY3_PROGRAM(alice_metrics_calculator_yt)

STRIP()

PEERDIR(
    alice/analytics/utils/yt
    alice/analytics/utils/auth
    alice/analytics/operations/priemka/metrics_calculator
    alice/analytics/operations/priemka/alice_parser/utils
    alice/analytics/tasks/VA-571

    contrib/python/click
    statbox/nile
    statbox/qb2
    yql/library/python
)

NO_CHECK_IMPORTS()

PY_SRCS(
    MAIN main.py
)

END()

RECURSE_ROOT_RELATIVE(
    alice/analytics/operations/priemka/metrics_calculator/udf
)
