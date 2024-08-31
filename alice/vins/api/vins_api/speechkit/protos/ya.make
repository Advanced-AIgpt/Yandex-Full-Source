PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    alexanderplat
    g:megamind
)

SRCS(
    vins_response.proto
)

PEERDIR(
    alice/megamind/protos/scenarios
)

EXCLUDE_TAGS(GO_PROTO)

END()
