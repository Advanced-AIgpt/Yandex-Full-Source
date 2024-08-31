PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

SRCS(
    module_serializer.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
