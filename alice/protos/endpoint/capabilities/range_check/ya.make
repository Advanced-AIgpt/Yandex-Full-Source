PROTO_LIBRARY()

OWNER(
    g:alice_iot
    g:yandex_io
)

SET(PROTOC_TRANSITIVE_HEADERS "no")

PEERDIR(
    alice/protos/endpoint
    mapreduce/yt/interface/protos
)

SRCS(
    capability.proto
)

END()
