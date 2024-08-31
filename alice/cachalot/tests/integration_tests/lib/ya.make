PY3_LIBRARY()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cachalot/tests/local_cachalot
    alice/cachalot/tests/ydb_tables
)

PY_SRCS(
    __init__.py
)

END()
