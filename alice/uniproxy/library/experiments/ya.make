PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/event_patcher
    alice/uniproxy/library/logging
    alice/uniproxy/library/settings
)

PY_SRCS(
    __init__.py
)

END()

RECURSE_FOR_TESTS(ut)
