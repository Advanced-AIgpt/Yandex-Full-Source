PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    deemonasd
    g:alice
)

PEERDIR(
    alice/protos/endpoint
    alice/protos/extensions
    mapreduce/yt/interface/protos
)

SRCS(
    route_manager.proto
)

END()
