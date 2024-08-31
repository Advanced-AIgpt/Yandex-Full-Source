YQL_PYTHON_UDF(retention)
REGISTER_YQL_PYTHON_UDF()

OWNER(
    gri201
    g:alice_analytics
)

PEERDIR(
    alice/analytics/operations/retention
)

PY_SRCS(
    __main__.py
)

END()
