PY3_PROGRAM(test_client_locator)

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/uniproxy/library/backends_memcached
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/messenger

    contrib/python/tornado/tornado-4
)

END()
