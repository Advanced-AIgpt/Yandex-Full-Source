PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:matrix
)

SRCS(
    subscriptions_config.proto
)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/protos/data/device
)

EXCLUDE_TAGS(GO_PROTO)

END()
