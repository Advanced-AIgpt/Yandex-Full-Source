PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    alice/megamind/library/stack_engine/protos
    alice/megamind/protos/guest
    alice/megamind/protos/scenarios

    alice/memento/proto
    alice/protos/data
)

SRCS(
    request_data.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
