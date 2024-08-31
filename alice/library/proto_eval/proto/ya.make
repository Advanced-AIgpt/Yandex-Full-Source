PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice
    lavv17
)

PEERDIR(
    mapreduce/yt/interface/protos
)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    expression.proto
)

END()
