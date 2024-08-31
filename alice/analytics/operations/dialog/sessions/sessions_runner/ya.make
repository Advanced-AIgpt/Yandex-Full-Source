YQL_PYTHON_UDF(sessions_runner)

REGISTER_YQL_PYTHON_UDF()

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/analytics/operations/dialog/sessions/lib_for_py2
    contrib/python/click
)

END()

RECURSE(
    utils_nirvana_workaround
)
