PROTO_LIBRARY()

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    mapreduce/yt/interface/protos
    alice/megamind/protos/common
    alice/protos/data
)

SRCS(
    voice_control.proto
)

END()
