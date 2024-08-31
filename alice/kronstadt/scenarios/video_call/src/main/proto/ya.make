PROTO_LIBRARY(video-call-protos)
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    video_call.proto
)

EXCLUDE_TAGS(GO_PROTO CPP_PROTO)

END()