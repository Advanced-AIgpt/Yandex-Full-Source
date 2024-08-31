PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice_iot
    g:yandex_io
)

PEERDIR(
    alice/protos/endpoint
    mapreduce/yt/interface/protos
)

SRCS(
    capability.proto
)

END()
