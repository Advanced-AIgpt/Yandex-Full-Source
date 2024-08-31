LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    directive_change_status_http_request.cpp
    directive_status_http_request.cpp
    service.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/storages/directives

    alice/matrix/library/request
    alice/matrix/library/services/iface
)

END()
