PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    video_nlu_meta.proto
)

END()
