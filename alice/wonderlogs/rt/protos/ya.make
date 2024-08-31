PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:wonderlogs)

SRCS(
    uniproxy_event.proto
    uuid_message_id.proto
)

PEERDIR(
    alice/wonderlogs/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
