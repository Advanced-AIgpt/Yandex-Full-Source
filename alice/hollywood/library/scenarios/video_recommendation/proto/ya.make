PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    dan-anastasev
    g:hollywood
)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    video_recommendation_state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
