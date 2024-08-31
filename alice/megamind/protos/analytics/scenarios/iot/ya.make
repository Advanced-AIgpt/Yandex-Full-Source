PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    g:alice_iot
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    iot.proto
)

END()
