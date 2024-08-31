PY3_PROGRAM()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/uniproxy/library/experiments
    alice/uniproxy/library/events
)

PY_SRCS(
    MAIN main.py
)

END()
