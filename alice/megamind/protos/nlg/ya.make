PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/censor/protos

    alice/megamind/protos/div

    mapreduce/yt/interface/protos
)

SRCS(
    nlg.proto
)

END()

