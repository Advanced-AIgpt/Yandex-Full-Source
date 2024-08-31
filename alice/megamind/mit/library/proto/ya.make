PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
    yagafarov
)

SRCS(
    dummy.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
