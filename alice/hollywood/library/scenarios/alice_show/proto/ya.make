PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
    lavv17
    g:alice
)

SRCS(
    config.proto
    fast_data.proto
    scenario_data.proto
    state.proto
)

PEERDIR(
    alice/hollywood/library/phrases/proto
    alice/hollywood/library/tags/proto
    alice/library/proto_eval/proto
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/data
    alice/protos/data/scenario/alice_show
)

EXCLUDE_TAGS(GO_PROTO)

END()
