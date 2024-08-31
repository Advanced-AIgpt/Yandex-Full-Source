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
    music_what_is_playing.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
