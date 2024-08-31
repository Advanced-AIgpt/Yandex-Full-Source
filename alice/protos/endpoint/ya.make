PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice_iot
    g:megamind
    g:yandex_io
)

PEERDIR(
    alice/protos/extensions
    mapreduce/yt/interface/protos
)

SRCS(
    capability.proto
    common.proto
    endpoint.proto
)

END()

RECURSE(
    capabilities
    events
    extensions
)
