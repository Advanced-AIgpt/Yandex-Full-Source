PY2_PROGRAM(sessions_purger)

OWNER(
    g:alice
    gusev-p
)

PEERDIR(
    ydb/public/sdk/python
    contrib/python/tornado/tornado-4
)

PY_SRCS(
    TOP_LEVEL
    __main__.py
)

END()
