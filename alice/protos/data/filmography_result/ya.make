PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/data/video
    mapreduce/yt/interface/protos
)

SRCS(
    filmography_result.proto
)

END()
