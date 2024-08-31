LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    service.cpp
    update_connected_clients_request.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/services/update_connected_clients/protos
    alice/matrix/notificator/library/storages/connections
    alice/matrix/notificator/library/storages/directives

    alice/matrix/library/request
    alice/matrix/library/services/iface
    alice/matrix/library/services/typed_apphost_service
)

END()

RECURSE(
    protos
)
