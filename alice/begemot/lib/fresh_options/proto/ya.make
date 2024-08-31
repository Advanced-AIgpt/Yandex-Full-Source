PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    samoylovboris
    g:alice_quality
    g:begemot
)

SRCS(
    fresh_options.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
