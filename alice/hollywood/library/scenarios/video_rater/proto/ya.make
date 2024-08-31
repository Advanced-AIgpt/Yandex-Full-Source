PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    ddale
    g:alice
)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    video_rater_state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
