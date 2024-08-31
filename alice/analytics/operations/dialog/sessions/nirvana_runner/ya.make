YQL_PYTHON_UDF(nirvana_runner)

OWNER(g:alice_analytics)

REGISTER_YQL_PYTHON_UDF()

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/analytics/operations/dialog/sessions/lib_for_py2
)

END()
