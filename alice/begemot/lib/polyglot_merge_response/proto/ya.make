PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    alexanderplat
    g:alice_quality
)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    config.proto
)

END()
