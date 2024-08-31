PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/logging
    alice/uniproxy/library/utils
)

PY_SRCS(
    __init__.py
)

END()

RECURSE_FOR_TESTS(ut)
RECURSE_ROOT_RELATIVE(alice/uniproxy/experiments)
