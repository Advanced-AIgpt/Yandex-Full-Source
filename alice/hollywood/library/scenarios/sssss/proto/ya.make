PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    akhruslan
    g:hollywood
)

SRCS(
    fast_data.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
