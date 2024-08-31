PROTO_LIBRARY()

SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/data/device
    mapreduce/yt/interface/protos
)

SRCS(
    group.proto
    room.proto
)

END()
