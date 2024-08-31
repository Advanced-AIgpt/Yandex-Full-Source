PY3_PROGRAM()

OWNER(
    g:matrix
    g:voicetech-infra
)

PY_SRCS(
    __main__.py
    rtlog_grip.py
)

PEERDIR(
    alice/uniproxy/library/notificator
    alice/uniproxy/library/profiling
)

END()
