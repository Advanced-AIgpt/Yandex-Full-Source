PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    igor-darov
    g:alice
)

SRCS(
    fast_data.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
