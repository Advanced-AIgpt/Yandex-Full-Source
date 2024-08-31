PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g-kostin
    g:alice
)

INCLUDE_TAGS(GO_PROTO)

GRPC()

SRCS(
    api.proto
    card.proto
    commands.proto
)

END()
