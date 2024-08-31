PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:matrix
)

SRCS(
    config.proto
)

PEERDIR(
    alice/matrix/library/config

    library/cpp/proto_config/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
