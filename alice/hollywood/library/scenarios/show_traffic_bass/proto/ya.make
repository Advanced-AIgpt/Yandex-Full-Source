PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    yagafarov
    g:hollywood
)

SRCS(
    show_traffic_bass.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
