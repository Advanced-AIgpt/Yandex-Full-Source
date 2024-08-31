PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    client_provider.py
    rtcdiscovery.py
)

PEERDIR(
    alice/uniproxy/library/logging
    alice/uniproxy/library/resolvers
    alice/uniproxy/library/settings
    alice/uniproxy/library/unimemcached
    alice/uniproxy/library/global_counter
)

END()

RECURSE_FOR_TESTS(ut)
