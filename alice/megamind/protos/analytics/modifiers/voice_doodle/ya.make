PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    voice_doodle.proto
)

END()
