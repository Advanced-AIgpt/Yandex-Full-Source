PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
    alkapov
)

PEERDIR(
    alice/megamind/protos/scenarios
)

SRCS(
    stack_engine.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
