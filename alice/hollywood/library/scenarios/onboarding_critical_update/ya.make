LIBRARY()

OWNER(
    jan-fazli
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/onboarding_critical_update/nlg
    alice/library/experiments
)

SRCS(
    GLOBAL onboarding_critical_update.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
