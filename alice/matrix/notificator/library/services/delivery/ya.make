LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    delivery_demo_http_request.cpp
    delivery_http_request.cpp
    delivery_on_connect_http_request.cpp
    delivery_push_http_request.cpp
    service.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/pushes_and_notifications
    alice/matrix/notificator/library/subscriptions_info
    alice/matrix/notificator/library/user_white_list
    alice/matrix/notificator/library/utils

    alice/matrix/library/request
    alice/matrix/library/services/iface

    alice/megamind/api/utils

    alice/protos/api/matrix

    alice/uniproxy/library/protos
)

END()
