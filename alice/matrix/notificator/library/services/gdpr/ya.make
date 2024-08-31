LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    gdpr_http_request.cpp
    service.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/pushes_and_notifications

    alice/matrix/library/request
    alice/matrix/library/services/iface
)

END()
