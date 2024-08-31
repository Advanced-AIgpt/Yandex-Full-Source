PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/uniproxy/library/logging
)

END()
