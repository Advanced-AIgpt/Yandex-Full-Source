PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/censor/protos
    alice/library/field_differ/protos

    alice/megamind/protos/analytics/combinators
    alice/megamind/protos/analytics/modifiers
    alice/megamind/protos/common
    alice/megamind/protos/modifiers
    alice/megamind/protos/scenarios

    mapreduce/yt/interface/protos
)

SRCS(
    analytics_info.proto
    megamind_analytics_info.proto
    recognized_action_info.proto
    user_info.proto
)

END()

RECURSE(
    combinators
    modifiers
    scenarios
)
