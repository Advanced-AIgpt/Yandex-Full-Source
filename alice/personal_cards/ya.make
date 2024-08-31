LIBRARY()

OWNER(
    g:personal-cards
)

PEERDIR(
    alice/bass/libs/logging

    alice/personal_cards/card
    alice/personal_cards/config
    alice/personal_cards/protos
    alice/personal_cards/push_cards_storage

    infra/libs/sensors

    kernel/reqid

    library/cpp/deprecated/split
    library/cpp/getopt
    library/cpp/http/server
    library/cpp/json
    library/cpp/proto_config
    library/cpp/protobuf/json
    library/cpp/sighandler
    library/cpp/threading/light_rw_lock
    library/cpp/tvmauth/client
)

SRCS(
    application.cpp
    http_handlers.cpp
    http_request.cpp
    ping_handler.cpp
    request_context.cpp
    rotate_logs_handler.cpp
    sensors.cpp
    server.cpp
    tvm.cpp
    version_handler.cpp
)

RESOURCE(
    alice/personal_cards/bin/config.json /proto_config/config.json
)

END()

RECURSE(
    bin
    monitoring
)

RECURSE_FOR_TESTS(
    canonize_tests
    integration_tests
)
