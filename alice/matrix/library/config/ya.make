PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:matrix
)

SRCS(
    config.proto
)

PEERDIR(
    library/cpp/proto_config/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
