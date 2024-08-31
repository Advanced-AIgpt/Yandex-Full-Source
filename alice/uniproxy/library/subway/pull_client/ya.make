PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    pull_client.py
    registry.py
    singleton.py
)

PEERDIR(
    alice/uniproxy/library/subway/common
    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/messenger
    alice/uniproxy/library/protos
    alice/uniproxy/library/settings
    alice/uniproxy/library/utils
    alice/uniproxy/library/notificator
)

END()
