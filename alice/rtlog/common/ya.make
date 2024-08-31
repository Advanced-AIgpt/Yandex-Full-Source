LIBRARY()

OWNER(gusev-p)

SRCS(
    service_instance_repository.cpp
    ydb_helpers.cpp
)

PEERDIR(
    alice/rtlog/protos
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
)

END()

RECURSE(
    eventlog
)
