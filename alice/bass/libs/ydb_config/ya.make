LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/ydb_helpers
    alice/bass/libs/ydb_kv
    ydb/public/sdk/cpp/client/ydb_table
)

SRC(config.cpp)

END()
