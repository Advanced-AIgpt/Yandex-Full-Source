LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/bass/libs/logging_v2
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_result
    ydb/public/sdk/cpp/client/ydb_scheme
    ydb/public/sdk/cpp/client/ydb_table
    ydb/public/sdk/cpp/client/ydb_value
)

SRCS(
    exception.cpp
    path.cpp
    protobuf.cpp
    queries.cpp
    settings.cpp
    table.cpp
    testing.cpp
    visitors.cpp
)

END()

RECURSE(
    ut
    ut_protos
)
