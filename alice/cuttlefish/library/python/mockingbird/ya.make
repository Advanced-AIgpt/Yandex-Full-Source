PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-6
    alice/cuttlefish/library/python/apphost_grpc_servant
)

PY_SRCS(
    __init__.py
    canonical.py
    server.py
    callback_handler.py
)

END()
