PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/stack_engine/protos
    alice/megamind/protos/analytics
    alice/megamind/protos/common
    alice/megamind/protos/modifiers
    alice/megamind/protos/proactivity
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    dj/services/alisa_skills/server/proto/client
)

SRCS(
    session.proto
    state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
