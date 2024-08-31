PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/global_counter
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
)

PY_SRCS(
    __init__.py
)

END()

RECURSE(
    ut
)
