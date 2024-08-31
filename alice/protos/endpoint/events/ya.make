PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice_iot
    g:megamind
    g:yandex_io
)

PEERDIR(
    alice/protos/endpoint
    mapreduce/yt/interface/protos
)

SRCS(
    events.proto
)

END()

RECURSE(
    all
)
