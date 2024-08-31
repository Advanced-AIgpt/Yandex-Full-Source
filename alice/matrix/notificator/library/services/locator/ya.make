LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    locator_http_request.cpp
    service.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/storages/locator

    alice/matrix/library/request
    alice/matrix/library/services/iface
)

END()
