PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    domain.proto
    serialized_grammar.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
