PY3_PROGRAM()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/testing
    library/python/vault_client
)

PY_SRCS(
    MAIN
    main.py
    tests/__init__.py
    tests/synchronize_state.py
)

END()
