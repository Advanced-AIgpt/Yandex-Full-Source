PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    deemonasd
    g:alice_boltalka
    g:megamind
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    general_conversation.proto
)

END()
