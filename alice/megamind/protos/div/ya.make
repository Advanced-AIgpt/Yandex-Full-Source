PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/censor/protos

    mapreduce/yt/interface/protos
)

SRCS(
    card.proto
    div2_cards.proto
)

END()

