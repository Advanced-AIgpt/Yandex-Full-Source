PROTO_LIBRARY()

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    div2id.proto
    div2card.proto
    div2patch.proto
    div2size.proto
)

END()
