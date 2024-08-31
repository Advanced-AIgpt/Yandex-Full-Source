PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    klim-roma
    g:alice
)

PEERDIR(
    alice/protos/data/scenario/objects
    mapreduce/yt/interface/protos
)

SRCS(
    personalization_data.proto
)

END()
