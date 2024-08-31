PY3_PROGRAM()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_message
)

PY_SRCS(
    MAIN
    main.py
)

END()
