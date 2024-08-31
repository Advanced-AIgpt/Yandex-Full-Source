PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:personal-cards
)

PEERDIR(
    library/cpp/proto_config/protos
)

SRCS(
    config.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
