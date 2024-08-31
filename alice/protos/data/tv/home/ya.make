PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/data/tv
    alice/protos/data/tv/tags
    alice/protos/data/video
    alice/protos/data/action
    mapreduce/yt/interface/protos
)

SRCS(
    request.proto
    result.proto
)

END()
