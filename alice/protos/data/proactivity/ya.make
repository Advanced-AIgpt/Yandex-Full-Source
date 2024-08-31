PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice_skill_recommendations
    lavv17
)

PEERDIR(
    alice/library/proto_eval/proto
    alice/megamind/protos/common
    mapreduce/yt/interface/protos
)

SRCS(
    analytics.proto
    last_views.proto
    tag_stats.proto
)

END()
