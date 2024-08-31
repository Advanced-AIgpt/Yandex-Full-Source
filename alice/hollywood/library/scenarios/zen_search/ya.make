LIBRARY()

OWNER(
    abc:zenfront
    mamay-igor
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/context
    alice/hollywood/library/frame
    alice/hollywood/library/global_context
    alice/hollywood/library/http_proxy
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/zen_search/nlg
    alice/library/proto
    alice/megamind/protos/common
)

SRCS(
    GLOBAL zen_search.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
