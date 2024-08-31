PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    deemonasd
    g:alice
)

SRCS(
    route_manager_scenario_state.proto
)

PEERDIR(
    alice/protos/endpoint/capabilities/route_manager
)

EXCLUDE_TAGS(GO_PROTO)

END()
