PY3_LIBRARY()

OWNER(
    g:matrix
    g:voicetech-infra
)

PY_SRCS(
    common_handler.py
    delivery.py
    notifications.py
    server.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/uniproxy/library/backends_common
    alice/uniproxy/library/backends_ctxs
    alice/uniproxy/library/events
    alice/uniproxy/library/subway/push_client
    alice/uniproxy/library/messenger
    alice/uniproxy/library/protos
    alice/uniproxy/library/personal_cards
    alice/uniproxy/library/resolvers
    alice/uniproxy/library/settings
    alice/uniproxy/library/common_handlers
    alice/uniproxy/library/notificator_api
)

END()
