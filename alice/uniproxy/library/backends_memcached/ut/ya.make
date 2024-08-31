PY3TEST()

OWNER(
    g:voicetech-infra
)

TEST_SRCS(
    client_provider_ut.py
)

PEERDIR(
    alice/uniproxy/library/backends_memcached
    alice/uniproxy/library/resolvers
    alice/uniproxy/library/settings
    alice/uniproxy/library/testing
)

END()
