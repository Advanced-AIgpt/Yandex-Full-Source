LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    notifications_change_status_http_request.cpp
    notifications_http_request.cpp
    service.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/pushes_and_notifications
    alice/matrix/notificator/library/subscriptions_info
    alice/matrix/notificator/library/utils

    alice/matrix/library/request
    alice/matrix/library/services/iface

    alice/uniproxy/library/protos

    library/cpp/cgiparam
)

END()
