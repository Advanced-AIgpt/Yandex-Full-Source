PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    alexanderplat
    g:hollywood
)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/vins/api/vins_api/speechkit/connectors/protocol/protos
)

SRCS(
    get_time.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
