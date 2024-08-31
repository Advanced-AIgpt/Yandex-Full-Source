PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    common_request_handler.py
    common_web_socket_handler.py
    hooks.py
    info.py
    internal.py
    ping.py
    unistat.py
    unknown.py
    utils.py
)

PEERDIR(
    contrib/python/tornado/tornado-4
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/auth
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/global_state
    alice/uniproxy/library/utils
)

END()
