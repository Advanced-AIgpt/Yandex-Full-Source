PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:marketinalice
)

SRCS(
    fast_data.proto
    report.proto
    search_info.proto
    types.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
