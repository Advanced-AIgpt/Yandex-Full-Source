PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    lvlasenkov
    g:milab
)

SRCS(
    resources.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
