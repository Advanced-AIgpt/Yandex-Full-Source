PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

SRCS(
    fst.proto
    custom.proto
)

PEERDIR(
    search/begemot/core/proto
)

END()
