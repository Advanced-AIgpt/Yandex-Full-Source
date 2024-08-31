PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    d-dima
    g:alice
)

PEERDIR(
    alice/megamind/protos/scenarios
)

SRCS(
    default_render.proto
    dummy_scene_args.proto
    framework_state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
