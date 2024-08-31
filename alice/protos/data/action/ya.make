PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    tv_action.proto
)

END()
