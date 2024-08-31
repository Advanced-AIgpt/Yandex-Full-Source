PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    deemonasd
    g:alice
    g:yandex_io
)

PEERDIR(
    alice/protos/data/channel
    alice/protos/endpoint
    alice/protos/extensions
    mapreduce/yt/interface/protos
)

SRCS(
    capability.proto
)

END()
