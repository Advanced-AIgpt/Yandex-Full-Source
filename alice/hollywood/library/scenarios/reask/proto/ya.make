PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    ardulat
    g:hollywood
    g:alice
)

SRCS(
    state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
