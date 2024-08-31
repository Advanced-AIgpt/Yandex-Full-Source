PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

SRCS(
    structs.proto
    test.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
