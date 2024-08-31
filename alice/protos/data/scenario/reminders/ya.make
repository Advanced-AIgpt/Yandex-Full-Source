PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice petrk)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    state.proto
    device_state.proto
)

END()
