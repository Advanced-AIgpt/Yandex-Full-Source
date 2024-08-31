YQL_PYTHON_UDF()
REGISTER_YQL_PYTHON_UDF()

OWNER(
    polinakud
)

PEERDIR(
    quality/ab_testing/cofe/projects/alice/heartbeats
    quality/ab_testing/cofe/projects/alice/timespent
    quality/ab_testing/cofe/projects/alice
    alice/analytics/operations/timespent
)

PY_SRCS(
    __main__.py
)

END()
