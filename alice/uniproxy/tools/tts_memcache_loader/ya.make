PY3_PROGRAM(memcache_loader)

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/uniproxy/library/logging
    alice/uniproxy/library/resolvers
    alice/uniproxy/library/settings
    alice/uniproxy/library/unimemcached
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/backends_memcached
    alice/uniproxy/library/backends_tts


)

END()
