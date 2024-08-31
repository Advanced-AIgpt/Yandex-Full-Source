PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    lavv17
    g:alice
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    selectors.proto
)

END()
