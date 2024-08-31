LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/ydb_helpers
    alice/bass/libs/ydb_kv/protos
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

SRC(kv.cpp)

END()

RECURSE(
    protos
    ut
)
