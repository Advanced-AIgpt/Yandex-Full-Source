LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    proxy_http_request.cpp
    sd_client.cpp
    service.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/library/request
    alice/matrix/library/services/iface

    infra/yp_service_discovery/libs/sdlib
    infra/yp_service_discovery/libs/sdlib/grpc_resolver

    library/cpp/neh/asio

    ydb/public/sdk/cpp/client/ydb_driver
)

END()
