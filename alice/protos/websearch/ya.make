PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    nkodosov
    g:megamind
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    websearch_parts.proto
)

END()
