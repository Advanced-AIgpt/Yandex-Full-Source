PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    vitvlkv
    g:hollywod
    g:alice
)

SRCS(
    zero_testing_state.proto
)

PEERDIR(
    alice/megamind/protos/scenarios
)

EXCLUDE_TAGS(GO_PROTO)

END()
