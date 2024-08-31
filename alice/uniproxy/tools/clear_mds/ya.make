PY3_PROGRAM(clear_mds)

OWNER(
    g:voicetech-infra
)

PEERDIR(
    alice/uniproxy/library/backends_common
    yt/python/client
)

PY_SRCS(
    __main__.py
)

END()
