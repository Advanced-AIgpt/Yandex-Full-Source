PY3_PROGRAM()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/apphost_grpc_client/ut_protos
)

PY_SRCS(
    MAIN
    main.py
)

END()
