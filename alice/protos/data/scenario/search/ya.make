PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/protos/data/scenario/objects
    mapreduce/yt/interface/protos
)

SRCS(
    fact.proto
    richcard.proto
    search_object.proto
)

END()
