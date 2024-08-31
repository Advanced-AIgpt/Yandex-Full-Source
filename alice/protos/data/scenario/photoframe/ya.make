PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smart_display)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    screen_saver.proto
)

END()
