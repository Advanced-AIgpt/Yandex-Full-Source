LIBRARY()

OWNER(
    the0
    g:megamind
)

PEERDIR(
    alice/library/logger/proto
    alice/library/logger
    alice/library/network
    alice/megamind/protos/scenarios
    kernel/server
    kernel/server/protos
    library/cpp/http/misc
    library/cpp/protobuf/json
)

SRCS(
    http_service.cpp
    protocol_scenario_http_service.cpp
    protocol_scenario.cpp
)

END()
