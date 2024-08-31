LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/zero_testing/nlg
    alice/hollywood/library/scenarios/zero_testing/proto
    alice/library/proto
)

SRCS(
    GLOBAL zero_testing.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
