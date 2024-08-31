PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice-alarm-scenario
    g:alice_scenarios
)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    reminders.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()

