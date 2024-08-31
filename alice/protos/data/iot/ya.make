PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice_iot)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/extensions
    mapreduce/yt/interface/protos
)

SRCS(
    scenario.proto
)

END()
