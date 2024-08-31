PY3_PROGRAM(migrator)

OWNER(
    g:matrix
)

PEERDIR(
    alice/protos/api/matrix

    ydb/public/sdk/python

    library/python/init_log

    contrib/python/requests
)

PY_SRCS(
    MAIN __main__.py
)

END()
