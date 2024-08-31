PY3_PROGRAM()

OWNER(
    g:voicetech-infra
)

ENABLE(NO_STRIP)

PY_SRCS(
    __main__.py
    rtlog_grip.py
)

PEERDIR(
    contrib/python/psutil
    contrib/python/tornado/tornado-4
    contrib/python/raven

    alice/uniproxy/library/auth
    alice/uniproxy/library/backends_memcached
    alice/uniproxy/library/common_handlers
    alice/uniproxy/library/delivery
    alice/uniproxy/library/extlog
    alice/uniproxy/library/frontend
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/global_state
    alice/uniproxy/library/legacy_navi
    alice/uniproxy/library/logging
    alice/uniproxy/library/messenger
    alice/uniproxy/library/profiling
    alice/uniproxy/library/settings
    alice/uniproxy/library/subway/pull_client
    alice/uniproxy/library/unisystem
    alice/uniproxy/library/web_handlers
    alice/uniproxy/library/tornado_speedups
)

END()

RECURSE_FOR_TESTS(tests)
