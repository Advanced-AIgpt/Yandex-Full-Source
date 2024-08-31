LIBRARY()

OWNER(
    lvlasenkov
    g:milab
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/global_context
    alice/hollywood/library/http_proxy
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/happy_new_year/nlg
    #alice/hollywood/library/scenarios/happy_new_year/resources/proto
)

SRCS(
    happy_new_year.cpp
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    it
)
