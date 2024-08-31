LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    service.cpp
    subscriptions_devices_http_request.cpp
    subscriptions_http_request.cpp
    subscriptions_manage_http_request.cpp
    subscriptions_user_list_http_request.cpp
)

PEERDIR(
    alice/matrix/notificator/library/services/common_context

    alice/matrix/notificator/library/pushes_and_notifications
    alice/matrix/notificator/library/subscriptions_info
    alice/matrix/notificator/library/user_white_list
    alice/matrix/notificator/library/utils

    alice/matrix/library/clients/iot_client
    alice/matrix/library/request
    alice/matrix/library/services/iface

    alice/uniproxy/library/protos

    library/cpp/cgiparam
    library/cpp/json
)

END()
