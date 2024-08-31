LIBRARY()

OWNER(
    artfulvampire
    g:developersyandextaxi
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/taximeter/nlg
    alice/library/proto
)

SRCS(
    GLOBAL requestconfirm.cpp
)

END()

RECURSE_FOR_TESTS(
    it
)
