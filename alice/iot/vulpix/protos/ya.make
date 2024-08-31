PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice_iot)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    apply_arguments.proto
    continue_arguments.proto
)

END()
