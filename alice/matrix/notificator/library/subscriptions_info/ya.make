LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    subscriptions_info.cpp
)

RESOURCE(
    alice/matrix/notificator/configs/subscriptions.json /subscriptions.json
)

PEERDIR(
    alice/matrix/notificator/library/subscriptions_info/protos

    alice/matrix/notificator/library/storages/connections

    library/cpp/protobuf/json
    library/cpp/resource
)

END()

RECURSE(
    protos
)

RECURSE_FOR_TESTS(
    ut
)
