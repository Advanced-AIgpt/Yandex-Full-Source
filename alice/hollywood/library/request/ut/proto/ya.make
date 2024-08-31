PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:hollywood)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    test_states.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
