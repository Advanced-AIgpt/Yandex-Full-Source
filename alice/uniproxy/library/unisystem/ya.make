PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    uniwebsocket.py
    unisystem.py
)

PEERDIR(
    alice/uniproxy/library/backends_laas
    alice/uniproxy/library/common_handlers
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/global_state
    alice/uniproxy/library/messenger
    alice/uniproxy/library/notificator_api
    alice/uniproxy/library/processors
    alice/uniproxy/library/subway/pull_client
    alice/uniproxy/library/utils
    alice/uniproxy/library/responses_storage
    contrib/python/raven
)

END()


RECURSE_FOR_TESTS(
    ut
)
