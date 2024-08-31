PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/data/action
    alice/protos/data/video
    alice/protos/div
    alice/protos/data/tv/carousel_item_config
    mapreduce/yt/interface/protos
)

SRCS(
    application.proto
    carousel.proto
    music.proto
)

END()

RECURSE(
    home
    carousel_item_config
    tags
    watch_list
    channels
)
