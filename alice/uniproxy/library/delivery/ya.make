PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    handler.py
    server.py
)

PEERDIR(
    alice/uniproxy/library/subway/push_client
    alice/uniproxy/library/messenger
    alice/uniproxy/library/resolvers
    alice/uniproxy/library/settings
    alice/uniproxy/library/common_handlers
    library/python/cityhash
)

END()
