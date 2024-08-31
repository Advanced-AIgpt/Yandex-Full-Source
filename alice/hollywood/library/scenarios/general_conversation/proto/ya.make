PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    deemonasd
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/hollywood/library/gif_card/proto
    alice/megamind/protos/common
    alice/protos/data/language
    alice/boltalka/libs/factors/proto
    alice/boltalka/generative/service/proto
)

SRCS(
    general_conversation.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
