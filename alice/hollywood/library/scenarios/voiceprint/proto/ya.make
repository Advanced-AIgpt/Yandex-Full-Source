PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice_scenarios
)

PEERDIR(
    alice/megamind/protos/common
    alice/protos/data/scenario/voiceprint
)

SRCS(
    voiceprint_arguments.proto
    voiceprint.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
