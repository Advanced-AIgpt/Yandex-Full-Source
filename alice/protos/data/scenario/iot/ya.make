PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/protos/data/scenario/objects
    mapreduce/yt/interface/protos
)

SRCS(
    enrollment_status.proto
)

END()
