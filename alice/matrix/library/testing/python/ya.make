PY3_LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice//cuttlefish/library/python/apphost_grpc_client

    ydb/public/sdk/python

    contrib/python/tornado/tornado-4

    library/python/sanitizers
)

PY_SRCS(
    helpers.py
    http_server_base.py
    servant_base.py
    tornado_io_loop.py
    tvm.py
    ydb.py
)

END()
