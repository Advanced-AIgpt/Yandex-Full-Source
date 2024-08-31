UNITTEST_FOR(alice/megamind/library/dispatcher)

OWNER(
    g:megamind
)

PEERDIR(
    alice/library/logger/proto
    alice/library/metrics
    alice/megamind/library/handlers/apphost_utility
    alice/megamind/library/registry
    alice/megamind/library/testing
    library/cpp/testing/unittest
    infra/udp_click_metrics/client
)

SRCS(
    apphost_dispatcher_ut.cpp
)

END()
