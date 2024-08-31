PY3_PROGRAM()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    alice/uniproxy/library/subway/server
)

END()
