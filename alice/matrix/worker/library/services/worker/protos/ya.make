PROTO_LIBRARY()

OWNER(
    g:matrix
)

EXCLUDE_TAGS(GO_PROTO)

PEERDIR(
    apphost/proto/extensions
    apphost/lib/grpc/protos
)

SRCS(
    private_api.proto
    service.proto
)

APPHOST()

END()
