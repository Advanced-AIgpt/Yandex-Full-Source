PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smarttv)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/div
    mapreduce/yt/interface/protos
)

SRCS(
    check_license.proto
)

END()
