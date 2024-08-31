LIBRARY()

OWNER(
    olegator
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/do_nothing/nlg
)

SRCS(
    GLOBAL do_nothing.cpp
)

END()
