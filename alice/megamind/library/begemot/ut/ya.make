UNITTEST_FOR(alice/megamind/library/begemot)

OWNER(g:megamind)

PEERDIR(
    alice/begemot/lib/api/experiments
    alice/begemot/lib/api/params
    alice/library/json
    alice/library/network
    alice/library/unittest
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/sources
    alice/megamind/library/testing
    alice/megamind/library/util
    alice/megamind/protos/scenarios
    library/cpp/testing/gmock_in_unittest
    search/begemot/rules/alice/session/proto
)

SRCS(
    begemot_ut.cpp
)

SIZE(MEDIUM)

END()
