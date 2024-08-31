PROTO_LIBRARY()

OWNER(
    g:matrix
)

EXCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/api/matrix

    apphost/proto/extensions
    apphost/lib/grpc/protos
)

SRCS(
    service.proto
)

APPHOST()

END()
