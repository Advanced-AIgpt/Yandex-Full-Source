PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
)

PEERDIR(
    alice/megamind/protos/scenarios
)

SRCS(
    frame_filler_request.proto
    frame_filler_state.proto
    scenario_response.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
