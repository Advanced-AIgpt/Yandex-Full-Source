LIBRARY()

OWNER(g:matrix)

SRCS(
    service.cpp
    update_device_environment_request.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/services/update_device_environment/protos

    alice/matrix/library/request
    alice/matrix/library/services/iface
    alice/matrix/library/services/typed_apphost_service

    ydb/public/sdk/cpp/client/ydb_driver
)

END()

RECURSE(
    protos
)
