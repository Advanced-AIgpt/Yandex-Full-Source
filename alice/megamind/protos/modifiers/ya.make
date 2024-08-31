PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind jan-fazli)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/client/protos
    alice/megamind/protos/analytics/modifiers
    alice/megamind/protos/proactivity
    alice/megamind/protos/scenarios
    alice/protos/data/language
    alice/protos/data
    mapreduce/yt/interface/protos
)

SRCS(
    modifiers.proto
    modifier_body.proto
    modifier_request.proto
    modifier_response.proto
)

END()
