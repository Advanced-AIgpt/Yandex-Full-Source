PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:marketinalice
)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

SRCS(
    apply_arguments.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
