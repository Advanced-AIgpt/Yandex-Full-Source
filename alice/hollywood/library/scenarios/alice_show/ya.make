LIBRARY()

OWNER(
    flimsywhimsy
    lavv17
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/phrases
    alice/hollywood/library/registry
    alice/hollywood/library/scenarios/alice_show/nlg
    alice/hollywood/library/scenarios/alice_show/proto
    alice/hollywood/library/scenarios/fast_command
    alice/hollywood/library/scenarios/music
    alice/hollywood/library/tags
    alice/library/analytics/common
    alice/library/experiments
    alice/library/proto
    alice/library/proto_eval
    alice/megamind/protos/scenarios
    contrib/libs/protobuf
    library/cpp/iterator
    library/cpp/protobuf/json
    library/cpp/string_utils/url
    library/cpp/timezone_conversion
)

SRCS(
    GLOBAL alice_show.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
)
