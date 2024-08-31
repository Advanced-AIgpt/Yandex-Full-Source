OWNER(g:alice_analytics)

YQL_PYTHON_UDF(alice_parser)

STRIP()

REGISTER_YQL_PYTHON_UDF()

PEERDIR(
    alice/analytics/operations/priemka/alice_parser/lib
)

PY_SRCS(
#    ../main.py
)

NO_CHECK_IMPORTS()

END()
