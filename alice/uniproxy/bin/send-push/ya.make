PY3_PROGRAM()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/requests

    alice/uniproxy/library/protos
)

END()
