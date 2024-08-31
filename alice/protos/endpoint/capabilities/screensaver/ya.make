PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:smarttv
)

PEERDIR(
    alice/protos/endpoint
    alice/protos/extensions
    mapreduce/yt/interface/protos
)

SRCS(
    capability.proto
)

END()
