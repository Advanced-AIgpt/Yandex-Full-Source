UNITTEST_FOR(alice/megamind/library/scenarios/protocol)

OWNER(g:megamind)

SIZE(MEDIUM)

PEERDIR(
    alice/library/analytics/scenario
    alice/library/network
    alice/library/unittest

    alice/megamind/library/analytics
    alice/megamind/library/experiments
    alice/megamind/library/request/event
    alice/megamind/library/request
    alice/megamind/library/testing

    apphost/lib/service_testing
)

SRCS(
    alice/megamind/library/scenarios/protocol/protocol_scenario_ut.cpp
)

END()
