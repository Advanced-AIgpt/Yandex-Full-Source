PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(movb)

SRCS(
    features.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()

