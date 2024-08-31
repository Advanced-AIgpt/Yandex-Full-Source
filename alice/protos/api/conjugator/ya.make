PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/protos/data/language
)

SRCS(
    api.proto
)

END()
