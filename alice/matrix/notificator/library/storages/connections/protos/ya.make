PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:matrix
)

SRCS(
    device_info.proto
)

PEERDIR(
    alice/protos/api/matrix
)

END()
