LIBRARY()

OWNER(
    tolyandex
    g:alice
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/environment_state
    alice/hollywood/library/frame
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/notifications/nlg
    alice/hollywood/library/scenarios/notifications/proto
    alice/library/json
    alice/library/logger
    alice/library/proto
    alice/protos/endpoint
)

SRCS(
    GLOBAL notifications.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
)
