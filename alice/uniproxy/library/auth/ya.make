PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    blackbox.py
    tvm2.py
    tvm_daemon_client.py
)

PEERDIR(
    alice/rtlog/client/python/lib

    alice/uniproxy/library/async_http_client
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/settings
    alice/uniproxy/library/logging

    contrib/python/aiohttp
    contrib/python/paramiko
    contrib/python/requests
    contrib/python/tornado/tornado-4

    library/python/deprecated/ticket_parser2
)

END()
