PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice-time-capsule-scenario
    g:alice_scenarios
)

PEERDIR(
    alice/megamind/protos/common
    alice/memento/proto
)

SRCS(
    time_capsule.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()

