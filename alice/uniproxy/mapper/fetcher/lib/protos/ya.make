PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice
)

NEED_CHECK()

SRCS(
    voice_input.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
