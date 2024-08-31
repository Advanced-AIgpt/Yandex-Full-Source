PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
)

END()

RECURSE_FOR_TESTS(ut)
