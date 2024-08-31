LIBRARY()

OWNER(g:matrix)

SRCS(
    service_runner.cpp
    service.cpp
)

PEERDIR(
    alice/matrix/library/clients/tvm_client
    alice/matrix/library/config
    alice/matrix/library/logging
    alice/matrix/library/metrics
    alice/matrix/library/mlock
    alice/matrix/library/rtlog
    alice/matrix/library/version

    alice/cuttlefish/library/apphost

    apphost/api/service/cpp

    library/cpp/proto_config

    ydb/public/sdk/cpp/client/ydb_driver
)

END()
