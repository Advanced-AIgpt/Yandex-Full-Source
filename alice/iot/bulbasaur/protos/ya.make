PROTO_LIBRARY()

OWNER(g:alice_iot)

SET(
    PROTOC_TRANSITIVE_HEADERS
    "no"
)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/megamind/protos/analytics/scenarios/iot
    alice/megamind/protos/common
    alice/protos/endpoint
)

SRCS(
    apply_arguments.proto
    capability.proto
    continue_arguments.proto
    device.proto
    device_type.proto
    group.proto
    megamind_states.proto
    property.proto
    room.proto
    scenario.proto
    sharing.proto
)

END()

RECURSE(
    apphost
)
