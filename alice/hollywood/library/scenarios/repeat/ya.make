LIBRARY()

OWNER(
    ikorobtsev
    g:megamind
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/nlg
    alice/hollywood/library/player_features
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/repeat/nlg
    alice/library/device_state
    alice/library/logger
)

SRCS(
    GLOBAL repeat.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
)
