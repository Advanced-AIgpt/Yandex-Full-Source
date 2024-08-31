PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    tolyandex
    g:alice
)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    similarity.proto
)

END()
