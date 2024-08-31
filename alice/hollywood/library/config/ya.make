PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    akhruslan
    g:hollywood
)

SRCS(
    config.proto
)

PEERDIR(
    alice/library/logger/proto
    library/cpp/getoptpb/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
