PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/protos/endpoint/capabilities/all
)

SRCS(
    required_messages.proto
)

END()
