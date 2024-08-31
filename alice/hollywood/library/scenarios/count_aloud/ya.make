LIBRARY()

OWNER(
    olegator
    g:alice_quality
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/frame
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/count_aloud/nlg
)

SRCS(
    GLOBAL count_aloud.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
