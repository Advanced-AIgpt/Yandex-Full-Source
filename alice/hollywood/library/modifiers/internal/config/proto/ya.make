PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    yagafarov
    g:hollywood
)

SRCS(
    exact_key_groups.proto
    exact_mapping_config.proto
)

PEERDIR(
    alice/library/client/protos
    alice/library/logger/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
