PY3_PROGRAM()

OWNER(
    g:voicetech-infra
)

PEERDIR(
    alice/cachalot/tests/local_cachalot
    alice/cachalot/tests/test_cases
    alice/cachalot/tests/ydb_tables
    alice/cuttlefish/tests/common
)

DEPENDS(
    alice/cachalot/bin
)

PY_SRCS(
    __main__.py
    env.py
)

END()
