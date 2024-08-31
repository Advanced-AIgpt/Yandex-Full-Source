LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    client.cpp
)

PEERDIR(
    alice/matrix/notificator/library/config
    alice/matrix/notificator/library/storages/connections
    alice/matrix/notificator/library/storages/directives
    alice/matrix/notificator/library/storages/locator
    alice/matrix/notificator/library/storages/notifications
    alice/matrix/notificator/library/storages/subscriptions
    alice/matrix/notificator/library/subscriptions_info
    alice/matrix/notificator/library/utils

    alice/matrix/library/clients/subway_client
    alice/matrix/library/config

    alice/library/proto
)

END()

RECURSE_FOR_TESTS(
    ut
)
