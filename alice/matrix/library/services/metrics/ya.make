LIBRARY()

OWNER(g:matrix)

SRCS(
    main_metrics_http_request.cpp
    main_metrics_service.cpp
    metrics_http_request_base.cpp
    ydb_metrics_http_request.cpp
    ydb_metrics_service.cpp
)

PEERDIR(
    alice/matrix/library/logging
    alice/matrix/library/metrics
    alice/matrix/library/rtlog
    alice/matrix/library/services/iface

    infra/libs/sensors

    ydb/public/sdk/cpp/client/extensions/solomon_stats
    ydb/public/sdk/cpp/client/ydb_driver
)

END()
