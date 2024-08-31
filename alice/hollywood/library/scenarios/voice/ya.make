LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/voice/nlg
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/memento/proto
    alice/protos/data
)

SRCS(
    GLOBAL voice.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
