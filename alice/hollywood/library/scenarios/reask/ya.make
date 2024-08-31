LIBRARY()

OWNER(
    ardulat
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/reask/nlg
    alice/hollywood/library/scenarios/reask/proto
    library/cpp/iterator
)

SRCS(
    GLOBAL reask.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
