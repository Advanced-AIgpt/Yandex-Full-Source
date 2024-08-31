PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
)

SRCS(
    vins.proto
)

PEERDIR(
    alice/megamind/protos/scenarios
)

EXCLUDE_TAGS(GO_PROTO)

END()
