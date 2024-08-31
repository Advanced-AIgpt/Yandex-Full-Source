PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
    g:alice
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    books.proto
    companies.proto
    image.proto
    music.proto
    person.proto
    places.proto
    text.proto
    video.proto
)

END()
