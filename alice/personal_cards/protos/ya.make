PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:personal-cards
)

SRCS(
    model.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
