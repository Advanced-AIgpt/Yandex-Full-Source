PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-4

    alice/library/python/decoder

    alice/uniproxy/library/auth
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
)

PY_SRCS(
    __init__.py
)

END()

RECURSE_FOR_TESTS(ut)
