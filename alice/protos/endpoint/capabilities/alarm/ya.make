PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice-scenario-alarm
    g:alice
    g:yandex_io
)

PEERDIR(
    alice/protos/data/scenario/alarm
    alice/protos/endpoint
    mapreduce/yt/interface/protos
)

SRCS(
    capability.proto
)

END()
