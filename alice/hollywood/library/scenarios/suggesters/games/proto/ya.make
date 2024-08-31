PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    dan-anastasev
    g:hollywood
)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    game_suggest_state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
