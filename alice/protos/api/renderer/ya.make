PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/protos/data/scenario
    alice/protos/div
)

SRCS(
    api.proto
)

END()
