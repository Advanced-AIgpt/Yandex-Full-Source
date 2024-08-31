PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:deemonasd)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    contacts.proto
)

END()
