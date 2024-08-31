LIBRARY()

OWNER(
    igor-darov
    g:alice
)

PEERDIR(
    alice/hollywood/library/scenarios/equalizer/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/request
    alice/hollywood/library/nlg
    alice/hollywood/library/base_scenario
    alice/megamind/protos/common
    alice/protos/endpoint
    alice/hollywood/library/environment_state
)

SRCS(
    GLOBAL equalizer.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
