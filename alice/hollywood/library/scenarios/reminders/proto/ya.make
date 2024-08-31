PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    petrk
    g:hollywod
    g:alice
)

SRCS(
    callback_payload.proto
    reminders.proto
)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/protos/data/scenario/reminders
    alice/vins/api/vins_api/speechkit/connectors/protocol/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
