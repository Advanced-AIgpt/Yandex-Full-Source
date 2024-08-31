PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice
    g:home
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    greetings.proto
    proactivity_teaser.proto
)

END()
