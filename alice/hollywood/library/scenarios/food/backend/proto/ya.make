PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    the0
    samoylovboris
    g:alice_quality
)

SRCS(
    requests.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
