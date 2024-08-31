PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    dan-anastasev
    g:alice_boltalka
    g:megamind
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    movie_akinator.proto
)

END()
