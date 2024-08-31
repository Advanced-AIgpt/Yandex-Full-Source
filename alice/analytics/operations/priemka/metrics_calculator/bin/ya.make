OWNER(g:alice_analytics)

PY3_PROGRAM(alice_metrics_calculator)

PEERDIR(
    contrib/python/click
    statbox/nile  # нужны для импортов из VA-571
    statbox/qb2

    alice/analytics/utils
    alice/analytics/utils/yt

    alice/analytics/operations/priemka/metrics_calculator
    alice/analytics/operations/priemka/alice_parser/utils
    alice/analytics/tasks/VA-571
)

NO_CHECK_IMPORTS()

PY_SRCS(
    MAIN main.py
)

END()
