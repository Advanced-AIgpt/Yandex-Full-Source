PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    datetime.proto 
)

END()
