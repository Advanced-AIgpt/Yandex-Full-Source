PY3_PROGRAM()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/python/apphost_here
    alice/cuttlefish/library/python/uniproxy2_daemon
)

PY_SRCS(
    MAIN
    __init__.py
    daemons.py
)

END()
