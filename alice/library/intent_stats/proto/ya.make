PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    tolyandex
    g:alice
)

SRCS(
    intent_stats.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
