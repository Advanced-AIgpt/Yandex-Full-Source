PROTO_LIBRARY()

SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    olegator
    g:alice_quality
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    response.proto
)

END()
