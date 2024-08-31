PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

PEERDIR(
    kernel/server/protos
)

SRCS(
    config.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()