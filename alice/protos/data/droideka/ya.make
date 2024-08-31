PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    droideka.proto
)

END()
