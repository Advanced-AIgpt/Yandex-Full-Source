PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/video_common/protos
    alice/protos/data/app_metrika
    mapreduce/yt/interface/protos
)

SRCS(
    card_detail.proto
    content_details.proto
    tv_backend_request.proto
    video.proto
    video_scenes.proto
)

END()
