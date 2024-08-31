YQL_PYTHON_UDF(core_spu)
REGISTER_YQL_PYTHON_UDF()

OWNER(
    gri201
    g:alice_analytics
)

PEERDIR(
    alice/analytics/operations/core_spu
)

PY_SRCS(
    __main__.py
)

END()
