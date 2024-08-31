OWNER(
    g-kostin
    g:megamind
)

PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

PEERDIR(
    alice/megamind/protos/scenarios
)

SRCS(
    features.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
