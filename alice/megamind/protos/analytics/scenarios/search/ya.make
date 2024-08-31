PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    g:megamind
    tolyandex
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    search.proto
)

END()
