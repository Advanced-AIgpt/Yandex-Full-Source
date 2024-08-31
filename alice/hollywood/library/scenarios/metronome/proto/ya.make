PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    nkodosov
    g:alice
)

SRCS(
    metronome_scenario_state.proto
    metronome_render_state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
