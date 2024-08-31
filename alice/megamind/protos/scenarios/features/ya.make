PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    alkapov
    g:megamind
)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/response_similarity/proto
)

SRCS(
    gc.proto
    music.proto
    search.proto
    vins.proto
)

END()
