PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    g:megamind
    g:paskills
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    dialogovo.proto
)

END()
