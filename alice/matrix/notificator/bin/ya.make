PROGRAM(matrix)

OWNER(
    g:matrix
)

SRCS(
    main.cpp
)

RESOURCE(
    alice/matrix/notificator/bin/config.json /proto_config/config.json
)

PEERDIR(
    alice/matrix/notificator/library/config
    alice/matrix/notificator/library/services/common_context
    alice/matrix/notificator/library/services/delivery
    alice/matrix/notificator/library/services/devices
    alice/matrix/notificator/library/services/directive
    alice/matrix/notificator/library/services/gdpr
    alice/matrix/notificator/library/services/locator
    alice/matrix/notificator/library/services/notifications
    alice/matrix/notificator/library/services/proxy
    alice/matrix/notificator/library/services/subscriptions
    alice/matrix/notificator/library/services/update_connected_clients
    alice/matrix/notificator/library/services/update_device_environment

    alice/matrix/library/daemon
    alice/matrix/library/services/metrics
)

END()
