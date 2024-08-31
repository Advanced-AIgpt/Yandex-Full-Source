UNITTEST_FOR(alice/hollywood/library/scenarios/test_scenario)

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/testing
    alice/hollywood/library/scenarios/test_scenario/proto
    alice/library/json
    apphost/lib/service_testing
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    test_scenario_render_ut.cpp
)

END()
