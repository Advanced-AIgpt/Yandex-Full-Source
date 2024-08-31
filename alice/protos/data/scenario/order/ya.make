PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/protos/data
    mapreduce/yt/interface/protos
)

SRCS(
    config.proto
    order.proto
)

END()
