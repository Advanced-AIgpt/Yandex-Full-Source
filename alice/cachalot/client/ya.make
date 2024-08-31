PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PEERDIR(
    alice/cachalot/api/protos
    alice/uniproxy/library/async_http_client
    apphost/lib/grpc/protos
    library/python/codecs
)

PY_SRCS(
    __init__.py
    activation.py
)

END()
