PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/protos/data/scenario/objects
    mapreduce/yt/interface/protos
)

SRCS(
    config.proto
    content_id.proto
    content_info.proto
    dj_request_data.proto
    infinite_feed.proto
    player.proto
    topic.proto
)

END()
