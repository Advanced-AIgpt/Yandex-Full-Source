PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
    g:megamind
)

SRCS(
    config.proto
)

PEERDIR(
    library/cpp/getoptpb/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
