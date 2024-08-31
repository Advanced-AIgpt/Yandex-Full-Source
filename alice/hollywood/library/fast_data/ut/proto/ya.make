PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
)

SRCS(
    test_fast_data.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
