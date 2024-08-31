YQL_PYTHON3_UDF(check_analytics_info)

OWNER(
    ran1s
    g:alice
)

REGISTER_YQL_PYTHON_UDF(
    NAME CustomPython3
)

PEERDIR(
    alice/acceptance/modules/check_analytics_info/lib
    contrib/python/click
    statbox/nile
    yt/python/yt/wrapper
    yql/library/python
)

PY_SRCS(
    __main__.py
)

END()
