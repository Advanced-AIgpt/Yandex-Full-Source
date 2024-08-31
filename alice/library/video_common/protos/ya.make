PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    tolyandex
    g:alice
)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/response_similarity/proto
)

SRCS(
    features.proto
    video_state.proto
)

END()
