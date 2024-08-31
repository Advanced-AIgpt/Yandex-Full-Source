PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    tolyandex
    g:hollywood
    g:alice
)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    notifications.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
