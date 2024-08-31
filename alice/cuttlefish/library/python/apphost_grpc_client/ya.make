PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    apphost/lib/grpc/protos
    alice/cuttlefish/library/python/apphost_message
)

PY_SRCS(
    __init__.py
    async_grpc_call.py
)

END()

RECURSE_FOR_TESTS(
    ut
)
