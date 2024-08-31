PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    server.py
    subway.py
)

PEERDIR(
    alice/uniproxy/library/common_handlers
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/utils
    alice/uniproxy/library/protos
    alice/uniproxy/library/subway/common
    alice/uniproxy/library/subway/pull_client
)

END()
