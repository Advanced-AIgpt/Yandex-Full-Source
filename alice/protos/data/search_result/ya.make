PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/data/tv
    alice/protos/data/video
    mapreduce/yt/interface/protos
)

SRCS(
    search_result.proto
    tv_search_result.proto
)

END()
