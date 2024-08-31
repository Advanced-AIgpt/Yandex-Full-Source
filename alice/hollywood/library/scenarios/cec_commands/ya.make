LIBRARY()

OWNER(
    vl-trifonov
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/cec_commands/nlg
)

SRCS(
    GLOBAL cec_commands.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
)
