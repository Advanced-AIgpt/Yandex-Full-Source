PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:matrix
)

SRCS(
   incompatible.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
