PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    d-dima
    g:alice
)

PEERDIR(
    alice/megamind/protos/common
    alice/protos/data/entities
)

SRCS(
    random_number_scenario_state.proto
    random_number_render_state.proto
    random_number_fastdata.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
