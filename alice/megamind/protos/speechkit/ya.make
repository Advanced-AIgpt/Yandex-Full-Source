PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    alice/library/censor/protos
    alice/library/client/protos
    alice/library/field_differ/protos
    alice/library/restriction_level/protos
    alice/megamind/protos/analytics
    alice/megamind/protos/blackbox
    alice/megamind/protos/common
    alice/megamind/protos/guest
    alice/megamind/protos/nlg
    alice/megamind/protos/proactivity
    alice/megamind/protos/quality_storage
    alice/megamind/protos/quasar
    alice/megamind/protos/scenarios
    alice/megamind/protos/div
    alice/protos/data

    mapreduce/yt/interface/protos
)

SRCS(
    directives.proto
    request.proto
    response.proto
)

END()
