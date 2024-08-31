OWNER(g:alice_analytics)

YQL_PYTHON3_UDF(alice_metrics_calculator_yt_udf)

STRIP()

REGISTER_YQL_PYTHON_UDF(
    NAME CustomPython3
)

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

END()
