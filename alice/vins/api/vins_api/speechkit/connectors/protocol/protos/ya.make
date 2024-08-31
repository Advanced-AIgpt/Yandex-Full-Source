PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    alkapov
    g:megamind
)

SRCS(
    state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
