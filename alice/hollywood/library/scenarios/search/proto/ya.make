PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
    g:alice
)

SRCS(
    apply_arguments.proto
    search.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
